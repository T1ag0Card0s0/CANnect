#include "cannect/cants/CanTsProtocol.hpp"

#include "CanTsCodec.hpp"
#include "cannect/Logger.hpp"
#include "cannect/Types.hpp"

#include <cstring>

using namespace cannect;
using C = CanTsCodec;

CanTsProtocol::CanTsProtocol(uint8_t localAddress)
    : localAddress(localAddress),
      setBlockMgr(localAddress,
                  [this](const CanTsHeader &h, const uint8_t *d, uint8_t dlc) { return sendFrame(h, d, dlc); }),
      getBlockMgr(localAddress,
                  [this](const CanTsHeader &h, const uint8_t *d, uint8_t dlc) { return sendFrame(h, d, dlc); })
{
}

Status CanTsProtocol::setFrameTransmitter(std::shared_ptr<ICanFrameTransmitter> tx)
{
    std::lock_guard<std::mutex> lock(mtx);
    transmitter = std::move(tx);
    return transmitter ? Status::SUCCESS : Status::INVALID_ARGUMENT;
}

void CanTsProtocol::setTelecommandHandler(TcHandler handler)
{
    std::lock_guard<std::mutex> lock(mtx);
    tcHandler = std::move(handler);
}

void CanTsProtocol::setTelemetryHandler(TmHandler handler)
{
    std::lock_guard<std::mutex> lock(mtx);
    tmHandler = std::move(handler);
}

void CanTsProtocol::setUnsolicitedTelemetryHandler(UTMHandler handler)
{
    std::lock_guard<std::mutex> lock(mtx);
    utmHandler = std::move(handler);
}

void CanTsProtocol::setTimeSyncHandler(TimeSyncHandler handler)
{
    std::lock_guard<std::mutex> lock(mtx);
    tsHandler = std::move(handler);
}

void CanTsProtocol::setGetBlockHandler(GetBlockHandler handler) { getBlockMgr.setHandler(std::move(handler)); }

void CanTsProtocol::setSetBlockHandler(SetBlockHandler handler) { setBlockMgr.setHandler(std::move(handler)); }

bool CanTsProtocol::sendFrame(const CanTsHeader &h, const uint8_t *data, uint8_t dlc)
{
    std::shared_ptr<ICanFrameTransmitter> tx;
    {
        std::lock_guard<std::mutex> lock(mtx);
        tx = transmitter;
    }

    if (!tx || dlc > CAN_FRAME_MAX_DATA)
    {
        return false;
    }

    CanFrame frame{};
    frame.id = C::encode(h);
    frame.dlc = dlc;

    if (data && dlc > 0)
    {
        std::memcpy(frame.data, data, dlc);
    }

    return tx->send(frame) == Status::SUCCESS;
}

void CanTsProtocol::notePeerAlive(uint8_t peer) { peers[peer].lastSeen = std::chrono::steady_clock::now(); }

Status CanTsProtocol::onFrame(const CanFrame &frame)
{
    const CanTsHeader header = C::decode(frame.id);

    {
        std::lock_guard<std::mutex> lock(mtx);
        notePeerAlive(header.from);

        if (header.to != localAddress && header.to != 0)
        {
            LOG_WARNING("CAN-TS frame addressed to another node");
            return Status::UNSUCCESS;
        }
    }

    switch (header.type)
    {
    case CanTsMessageType::TELECOMMAND:
        handleTelecommand(header, frame);
        break;
    case CanTsMessageType::TELEMETRY:
        handleTelemetry(header, frame);
        break;
    case CanTsMessageType::UNSOLICITED:
        handleUnsolicited(header, frame);
        break;
    case CanTsMessageType::TIMESYNC:
        handleTimeSync(header, frame);
        break;
    case CanTsMessageType::SETBLOCK:
        setBlockMgr.onFrame(header, frame);
        break;
    case CanTsMessageType::GETBLOCK:
        getBlockMgr.onFrame(header, frame);
        break;
    default:
        break;
    }

    return Status::SUCCESS;
}

void CanTsProtocol::handleTelecommand(const CanTsHeader &h, const CanFrame &frame)
{
    const auto kind = C::getTcTmKind(h.command);
    const uint8_t channel = C::getChannel(h.command);

    if (kind == CanTsReqAck::REQ)
    {
        TcHandler handlerCopy;
        {
            std::lock_guard<std::mutex> lock(mtx);
            handlerCopy = tcHandler;
        }

        uint8_t req[CAN_FRAME_MAX_DATA] = {};
        std::memcpy(req, frame.data, frame.dlc);

        bool ok = false;
        if (handlerCopy)
        {
            ok = handlerCopy(h.from, channel, req);
        }

        CanTsHeader resp{};
        resp.to = h.from;
        resp.from = localAddress;
        resp.type = CanTsMessageType::TELECOMMAND;
        resp.command = C::makeTcTmCommand(ok ? CanTsReqAck::ACK : CanTsReqAck::NACK, channel);

        (void)sendFrame(resp, frame.data, frame.dlc);
        return;
    }

    if (kind == CanTsReqAck::ACK || kind == CanTsReqAck::NACK)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (pendingTc.waiting && pendingTc.peer == h.from && pendingTc.channel == channel)
        {
            pendingTc.done = true;
            pendingTc.ok = (kind == CanTsReqAck::ACK);
            pendingTc.dlc = frame.dlc;
            std::memcpy(pendingTc.data, frame.data, frame.dlc);
            cv.notify_all();
        }
    }
}

void CanTsProtocol::handleTelemetry(const CanTsHeader &h, const CanFrame &frame)
{
    const auto kind = C::getTcTmKind(h.command);
    const uint8_t channel = C::getChannel(h.command);

    if (kind == CanTsReqAck::REQ)
    {
        TmHandler handlerCopy;
        {
            std::lock_guard<std::mutex> lock(mtx);
            handlerCopy = tmHandler;
        }

        uint8_t respData[CAN_FRAME_MAX_DATA] = {};
        bool ok = false;

        if (handlerCopy)
        {
            ok = handlerCopy(h.from, channel, respData);
        }

        CanTsHeader resp{};
        resp.to = h.from;
        resp.from = localAddress;
        resp.type = CanTsMessageType::TELEMETRY;
        resp.command = C::makeTcTmCommand(ok ? CanTsReqAck::ACK : CanTsReqAck::NACK, channel);

        (void)sendFrame(resp, ok ? respData : nullptr, ok ? CAN_FRAME_MAX_DATA : 0);
        return;
    }

    if (kind == CanTsReqAck::ACK || kind == CanTsReqAck::NACK)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (pendingTm.waiting && pendingTm.peer == h.from && pendingTm.channel == channel)
        {
            pendingTm.done = true;
            pendingTm.ok = (kind == CanTsReqAck::ACK);
            pendingTm.dlc = frame.dlc;
            std::memcpy(pendingTm.data, frame.data, frame.dlc);
            std::memcpy(lastTelemetry, frame.data, frame.dlc);
            lastTelemetryDlc = frame.dlc;
            cv.notify_all();
        }
    }
}

void CanTsProtocol::handleUnsolicited(const CanTsHeader &h, const CanFrame &frame)
{
    UTMHandler handlerCopy;
    {
        std::lock_guard<std::mutex> lock(mtx);
        handlerCopy = utmHandler;
    }

    if (!handlerCopy)
    {
        return;
    }

    uint8_t buf[CAN_FRAME_MAX_DATA] = {};
    std::memcpy(buf, frame.data, frame.dlc);
    handlerCopy(h.from, C::getChannel(h.command), buf);
}

void CanTsProtocol::handleTimeSync(const CanTsHeader &h, const CanFrame &frame)
{
    TimeSyncHandler handlerCopy;
    {
        std::lock_guard<std::mutex> lock(mtx);
        handlerCopy = tsHandler;
    }

    if (!handlerCopy)
    {
        return;
    }

    uint8_t buf[CAN_FRAME_MAX_DATA] = {};
    std::memcpy(buf, frame.data, frame.dlc);
    handlerCopy(h.from, buf);
}

bool CanTsProtocol::sendTelecommand(uint8_t to, uint8_t channel, const uint8_t data[CAN_FRAME_MAX_DATA])
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (pendingTc.waiting)
        {
            return false;
        }

        pendingTc = {};
        pendingTc.waiting = true;
        pendingTc.peer = to;
        pendingTc.channel = channel;
    }

    const CanTsHeader h{to, localAddress, CanTsMessageType::TELECOMMAND, C::makeTcTmCommand(CanTsReqAck::REQ, channel)};

    if (!sendFrame(h, data, CAN_FRAME_MAX_DATA))
    {
        std::lock_guard<std::mutex> lock(mtx);
        pendingTc = {};
        return false;
    }

    std::unique_lock<std::mutex> lock(mtx);
    const bool completed =
        cv.wait_for(lock, std::chrono::milliseconds(DEFAULT_TIMEOUT_MS), [&] { return pendingTc.done; });

    const bool ok = completed && pendingTc.ok;
    pendingTc = {};
    return ok;
}

bool CanTsProtocol::requestTelemetry(uint8_t to, uint8_t channel)
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (pendingTm.waiting)
        {
            return false;
        }

        pendingTm = {};
        pendingTm.waiting = true;
        pendingTm.peer = to;
        pendingTm.channel = channel;
    }

    const CanTsHeader h{to, localAddress, CanTsMessageType::TELEMETRY, C::makeTcTmCommand(CanTsReqAck::REQ, channel)};

    if (!sendFrame(h, nullptr, 0))
    {
        std::lock_guard<std::mutex> lock(mtx);
        pendingTm = {};
        return false;
    }

    std::unique_lock<std::mutex> lock(mtx);
    const bool completed =
        cv.wait_for(lock, std::chrono::milliseconds(DEFAULT_TIMEOUT_MS), [&] { return pendingTm.done; });

    const bool ok = completed && pendingTm.ok;
    pendingTm = {};
    return ok;
}

bool CanTsProtocol::sendUnsolicitedTelemetry(uint8_t to, uint8_t channel, const uint8_t data[CAN_FRAME_MAX_DATA])
{
    const CanTsHeader h{to, localAddress, CanTsMessageType::UNSOLICITED, static_cast<uint16_t>(channel)};
    return sendFrame(h, data, CAN_FRAME_MAX_DATA);
}

bool CanTsProtocol::broadcastTimeSync(const uint8_t timeLE[CAN_FRAME_MAX_DATA])
{
    const CanTsHeader h{0, localAddress, CanTsMessageType::TIMESYNC, 0};
    return sendFrame(h, timeLE, CAN_FRAME_MAX_DATA);
}

bool CanTsProtocol::setBlock(uint8_t to, const std::vector<uint8_t> &addressLE, const std::vector<uint8_t> &data,
                             uint32_t timeoutMs)
{
    return setBlockMgr.setBlock(to, addressLE, data, timeoutMs);
}

bool CanTsProtocol::getBlock(uint8_t to, const std::vector<uint8_t> &addressLE, uint32_t timeoutMs)
{
    return getBlockMgr.getBlock(to, addressLE, timeoutMs);
}

bool CanTsProtocol::getLastTelemetry(uint8_t out[CAN_FRAME_MAX_DATA], uint8_t &dlc) const
{
    std::lock_guard<std::mutex> lock(mtx);
    if (lastTelemetryDlc == 0)
    {
        return false;
    }

    std::memcpy(out, lastTelemetry, lastTelemetryDlc);
    dlc = lastTelemetryDlc;
    return true;
}

bool CanTsProtocol::getLastGetBlockData(std::vector<uint8_t> &out) const { return getBlockMgr.getLastData(out); }

bool CanTsProtocol::isPeerConnected(uint8_t peer, uint32_t timeoutMs) const
{
    std::lock_guard<std::mutex> lock(mtx);
    const auto it = peers.find(peer);
    if (it == peers.end())
    {
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.lastSeen).count() <= timeoutMs;
}

bool CanTsProtocol::sendKeepAlive(uint8_t channel, const uint8_t data[CAN_FRAME_MAX_DATA])
{
    return sendUnsolicitedTelemetry(CAN_TS_KEEPALIVE_ADDRESS, channel, data);
}
