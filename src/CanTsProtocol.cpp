#include "CanTsProtocol.hpp"

#include "cannect/Logger.hpp"
#include "cannect/Types.hpp"

#include <algorithm>
#include <cstring>

using namespace cannect;

namespace
{
class EncoderDecoder
{
  public:
    static constexpr uint32_t TO_SHIFT = 21;
    static constexpr uint32_t TYPE_SHIFT = 18;
    static constexpr uint32_t FROM_SHIFT = 10;
    static constexpr uint32_t CMD_SHIFT = 0;

    static constexpr uint32_t TO_MASK = 0xFFu << TO_SHIFT;
    static constexpr uint32_t TYPE_MASK = 0x7u << TYPE_SHIFT;
    static constexpr uint32_t FROM_MASK = 0xFFu << FROM_SHIFT;
    static constexpr uint32_t CMD_MASK = 0x3FFu << CMD_SHIFT;

    static uint32_t encode(const CanTsHeader &h)
    {
        uint32_t id = 0;
        id |= (uint32_t(h.to) << TO_SHIFT) & TO_MASK;
        id |= (uint32_t(h.type) << TYPE_SHIFT) & TYPE_MASK;
        id |= (uint32_t(h.from) << FROM_SHIFT) & FROM_MASK;
        id |= (uint32_t(h.command) << CMD_SHIFT) & CMD_MASK;
        return id;
    }

    static CanTsHeader decode(uint32_t can_id)
    {
        CanTsHeader h{};
        h.to = uint8_t((can_id & TO_MASK) >> TO_SHIFT);
        h.type = static_cast<CanTsMessageType>((can_id & TYPE_MASK) >> TYPE_SHIFT);
        h.from = uint8_t((can_id & FROM_MASK) >> FROM_SHIFT);
        h.command = uint16_t((can_id & CMD_MASK) >> CMD_SHIFT);
        return h;
    }
};

constexpr uint16_t TC_TM_KIND_SHIFT = 8;
constexpr uint16_t TC_TM_KIND_MASK = 0x3u << TC_TM_KIND_SHIFT;
constexpr uint16_t CHANNEL_MASK = 0xFFu;

constexpr uint16_t BLOCK_KIND_SHIFT = 7;
constexpr uint16_t BLOCK_KIND_MASK = 0x7u << BLOCK_KIND_SHIFT;
constexpr uint16_t BLOCK_LOW6_MASK = 0x3Fu;
constexpr uint16_t BLOCK_LOW7_MASK = 0x7Fu;
constexpr uint16_t SETBLOCK_DONE_MASK = 1u << 6;

inline uint16_t makeTcTmCommand(CanTsReqAck kind, uint8_t channel)
{
    return (static_cast<uint16_t>(kind) << TC_TM_KIND_SHIFT) | uint16_t(channel);
}

inline CanTsReqAck getTcTmKind(uint16_t command)
{
    return static_cast<CanTsReqAck>((command & TC_TM_KIND_MASK) >> TC_TM_KIND_SHIFT);
}

inline uint8_t getChannel(uint16_t command) { return static_cast<uint8_t>(command & CHANNEL_MASK); }

inline uint16_t makeSetBlockCommand(SetBlockFrameType type, uint8_t low6)
{
    return (static_cast<uint16_t>(type) << BLOCK_KIND_SHIFT) | (uint16_t(low6) & BLOCK_LOW6_MASK);
}

inline uint16_t makeSetBlockReportCommand(uint8_t low6, bool done)
{
    uint16_t cmd = makeSetBlockCommand(SetBlockFrameType::REPORT, low6);
    if (done)
    {
        cmd |= SETBLOCK_DONE_MASK;
    }
    return cmd;
}

inline SetBlockFrameType getSetBlockType(uint16_t command)
{
    return static_cast<SetBlockFrameType>((command & BLOCK_KIND_MASK) >> BLOCK_KIND_SHIFT);
}

inline uint8_t getSetBlockLow6(uint16_t command) { return static_cast<uint8_t>(command & BLOCK_LOW6_MASK); }

inline bool getSetBlockDone(uint16_t command) { return (command & SETBLOCK_DONE_MASK) != 0; }

inline uint16_t makeGetBlockCommand(GetBlockFrameType type, uint8_t low6)
{
    return (static_cast<uint16_t>(type) << BLOCK_KIND_SHIFT) | (uint16_t(low6) & BLOCK_LOW6_MASK);
}

inline GetBlockFrameType getGetBlockType(uint16_t command)
{
    return static_cast<GetBlockFrameType>((command & BLOCK_KIND_MASK) >> BLOCK_KIND_SHIFT);
}

inline uint8_t getGetBlockLow6(uint16_t command) { return static_cast<uint8_t>(command & BLOCK_LOW6_MASK); }

inline uint16_t ackCommandLow7(uint16_t command) { return command & BLOCK_LOW7_MASK; }
} // namespace

CanTsProtocol::CanTsProtocol(uint8_t localAddress) : localAddress(localAddress) {}

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

void CanTsProtocol::setGetBlockHandler(GetBlockHandler handler)
{
    std::lock_guard<std::mutex> lock(mtx);
    getBlockHandler = std::move(handler);
}

void CanTsProtocol::setSetBlockHandler(SetBlockHandler handler)
{
    std::lock_guard<std::mutex> lock(mtx);
    setBlockHandler = std::move(handler);
}

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
    frame.id = EncoderDecoder::encode(h);
    frame.dlc = dlc;

    if (data && dlc > 0)
    {
        std::memcpy(frame.data, data, dlc);
    }

    return tx->send(frame) == Status::SUCCESS;
}

void CanTsProtocol::notePeerAlive(uint8_t peer) { peers[peer].lastSeen = std::chrono::steady_clock::now(); }

size_t CanTsProtocol::bitmapSizeBytes(uint8_t numBlocks) { return (static_cast<size_t>(numBlocks) + 7u) / 8u; }

bool CanTsProtocol::allBlocksReceived(const std::vector<bool> &received)
{
    return std::all_of(received.begin(), received.end(), [](bool v) { return v; });
}

std::vector<uint8_t> CanTsProtocol::makeBitmap(const std::vector<bool> &received)
{
    std::vector<uint8_t> bitmap((received.size() + 7u) / 8u, 0u);
    for (size_t i = 0; i < received.size(); ++i)
    {
        if (received[i])
        {
            bitmap[i / 8u] |= static_cast<uint8_t>(1u << (i % 8u));
        }
    }
    return bitmap;
}

bool CanTsProtocol::bitmapBitIsSet(const std::vector<uint8_t> &bitmap, uint8_t index)
{
    const size_t byteIndex = index / 8u;
    const uint8_t bitIndex = index % 8u;

    if (byteIndex >= bitmap.size())
    {
        return false;
    }

    return (bitmap[byteIndex] & static_cast<uint8_t>(1u << bitIndex)) != 0;
}

Status CanTsProtocol::onFrame(const CanFrame &frame)
{
    const CanTsHeader header = EncoderDecoder::decode(frame.id);

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
        handleSetBlock(header, frame);
        break;
    case CanTsMessageType::GETBLOCK:
        handleGetBlock(header, frame);
        break;
    default:
        break;
    }

    return Status::SUCCESS;
}

void CanTsProtocol::handleTelecommand(const CanTsHeader &h, const CanFrame &frame)
{
    const auto kind = getTcTmKind(h.command);
    const uint8_t channel = getChannel(h.command);

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
        resp.command = makeTcTmCommand(ok ? CanTsReqAck::ACK : CanTsReqAck::NACK, channel);

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
    const auto kind = getTcTmKind(h.command);
    const uint8_t channel = getChannel(h.command);

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
        resp.command = makeTcTmCommand(ok ? CanTsReqAck::ACK : CanTsReqAck::NACK, channel);

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
    handlerCopy(h.from, getChannel(h.command), buf);
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

void CanTsProtocol::handleSetBlock(const CanTsHeader &h, const CanFrame &frame)
{
    const SetBlockFrameType type = getSetBlockType(h.command);

    switch (type)
    {
    case SetBlockFrameType::REQUEST: {
        const uint8_t numBlocks = static_cast<uint8_t>(getSetBlockLow6(h.command) + 1u);

        bool accepted = false;
        {
            std::lock_guard<std::mutex> lock(mtx);

            BlockSession session{};
            session.peer = h.from;
            session.channel = 0;
            session.address.assign(frame.data, frame.data + frame.dlc);
            session.numBlocks = numBlocks;
            session.blocks.assign(numBlocks, {});
            session.lastActivity = std::chrono::steady_clock::now();
            session.prepared = true;
            session.done = false;
            session.accepted = (numBlocks > 0 && numBlocks <= 64 &&
                                static_cast<size_t>(numBlocks) * CAN_FRAME_MAX_DATA <= CAN_TS_MAX_BLOCK_BYTES &&
                                !session.address.empty());

            accepted = session.accepted;
            sbSessions[h.from] = std::move(session);
        }

        CanTsHeader resp{};
        resp.to = h.from;
        resp.from = localAddress;
        resp.type = CanTsMessageType::SETBLOCK;
        resp.command =
            makeSetBlockCommand(accepted ? SetBlockFrameType::ACK : SetBlockFrameType::NACK, ackCommandLow7(h.command));

        (void)sendFrame(resp, frame.data, frame.dlc);
        break;
    }

    case SetBlockFrameType::TRANSFER: {
        bool accept = false;
        uint8_t seq = getSetBlockLow6(h.command);

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = sbSessions.find(h.from);
            if (it != sbSessions.end() && it->second.accepted && seq < it->second.numBlocks)
            {
                it->second.blocks[seq].assign(frame.data, frame.data + frame.dlc);
                it->second.lastActivity = std::chrono::steady_clock::now();
                accept = true;
            }
        }

        CanTsHeader resp{};
        resp.to = h.from;
        resp.from = localAddress;
        resp.type = CanTsMessageType::SETBLOCK;
        resp.command =
            makeSetBlockCommand(accept ? SetBlockFrameType::ACK : SetBlockFrameType::NACK, ackCommandLow7(h.command));

        (void)sendFrame(resp, frame.data, frame.dlc);
        break;
    }

    case SetBlockFrameType::STATUS_REQUEST: {
        SetBlockHandler handlerCopy;
        std::vector<uint8_t> assembled;
        std::vector<uint8_t> bitmap;
        bool haveSession = false;
        bool accepted = false;
        bool alreadyDone = false;

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = sbSessions.find(h.from);
            if (it != sbSessions.end())
            {
                haveSession = true;
                accepted = it->second.accepted;
                alreadyDone = it->second.done;
                handlerCopy = setBlockHandler;

                if (accepted)
                {
                    std::vector<bool> received(it->second.numBlocks, false);
                    for (uint8_t i = 0; i < it->second.numBlocks; ++i)
                    {
                        received[i] = !it->second.blocks[i].empty();
                    }

                    bitmap = makeBitmap(received);

                    if (allBlocksReceived(received) && !it->second.done)
                    {
                        assembled.reserve(static_cast<size_t>(it->second.numBlocks) * CAN_FRAME_MAX_DATA);
                        for (uint8_t i = 0; i < it->second.numBlocks; ++i)
                        {
                            assembled.insert(assembled.end(), it->second.blocks[i].begin(), it->second.blocks[i].end());
                        }
                    }
                }
            }
        }

        if (!haveSession || !accepted)
        {
            CanTsHeader nack{};
            nack.to = h.from;
            nack.from = localAddress;
            nack.type = CanTsMessageType::SETBLOCK;
            nack.command = makeSetBlockCommand(SetBlockFrameType::NACK, ackCommandLow7(h.command));
            (void)sendFrame(nack, frame.data, frame.dlc);
            return;
        }

        bool handlerDone = alreadyDone;
        if (!assembled.empty() && !alreadyDone)
        {
            if (handlerCopy)
            {
                std::vector<uint8_t> request = assembled;
                handlerDone = handlerCopy(h.from, 0, request);
            }
            else
            {
                handlerDone = true;
            }

            std::lock_guard<std::mutex> lock(mtx);
            auto it = sbSessions.find(h.from);
            if (it != sbSessions.end())
            {
                it->second.done = handlerDone;
            }
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = sbSessions.find(h.from);
            if (it != sbSessions.end())
            {
                std::vector<bool> received(it->second.numBlocks, false);
                for (uint8_t i = 0; i < it->second.numBlocks; ++i)
                {
                    received[i] = !it->second.blocks[i].empty();
                }
                bitmap = makeBitmap(received);
                handlerDone = it->second.done;
            }
        }

        CanTsHeader report{};
        report.to = h.from;
        report.from = localAddress;
        report.type = CanTsMessageType::SETBLOCK;
        report.command = makeSetBlockReportCommand(static_cast<uint8_t>(bitmap.size() & BLOCK_LOW6_MASK), handlerDone);

        const uint8_t dlc = static_cast<uint8_t>(std::min<size_t>(bitmap.size(), CAN_FRAME_MAX_DATA));
        (void)sendFrame(report, bitmap.data(), dlc);
        break;
    }

    case SetBlockFrameType::ABORT: {
        {
            std::lock_guard<std::mutex> lock(mtx);
            sbSessions.erase(h.from);
        }

        CanTsHeader ack{};
        ack.to = h.from;
        ack.from = localAddress;
        ack.type = CanTsMessageType::SETBLOCK;
        ack.command = makeSetBlockCommand(SetBlockFrameType::ACK, ackCommandLow7(h.command));

        (void)sendFrame(ack, frame.data, frame.dlc);
        break;
    }

    case SetBlockFrameType::ACK: {
        std::lock_guard<std::mutex> lock(mtx);
        if (pendingSb.active && pendingSb.peer == h.from && pendingSb.waitingRequestAck)
        {
            pendingSb.requestAcked = true;
            pendingSb.waitingRequestAck = false;
            cv.notify_all();
        }
        break;
    }

    case SetBlockFrameType::NACK: {
        std::lock_guard<std::mutex> lock(mtx);
        if (pendingSb.active && pendingSb.peer == h.from)
        {
            if (pendingSb.waitingRequestAck || pendingSb.waitingReport)
            {
                pendingSb.done = true;
                pendingSb.ok = false;
                cv.notify_all();
            }
        }
        break;
    }

    case SetBlockFrameType::REPORT: {
        std::lock_guard<std::mutex> lock(mtx);
        if (pendingSb.active && pendingSb.peer == h.from && pendingSb.waitingReport)
        {
            pendingSb.bitmap.assign(frame.data, frame.data + frame.dlc);
            pendingSb.done = getSetBlockDone(h.command);
            pendingSb.ok = true;
            pendingSb.reportReceived = true;
            cv.notify_all();
        }
        break;
    }

    default:
        break;
    }
}

void CanTsProtocol::handleGetBlock(const CanTsHeader &h, const CanFrame &frame)
{
    const GetBlockFrameType type = getGetBlockType(h.command);

    switch (type)
    {
    case GetBlockFrameType::REQUEST: {
        const uint8_t numBlocks = static_cast<uint8_t>(getGetBlockLow6(h.command) + 1u);

        GetBlockHandler handlerCopy;
        {
            std::lock_guard<std::mutex> lock(mtx);
            handlerCopy = getBlockHandler;
        }

        std::vector<uint8_t> responseData;
        bool ok = false;

        if (handlerCopy)
        {
            ok = handlerCopy(h.from, 0, responseData);
        }

        if (!ok || numBlocks == 0 || numBlocks > 64 || responseData.size() > CAN_TS_MAX_BLOCK_BYTES ||
            responseData.size() != static_cast<size_t>(numBlocks) * CAN_FRAME_MAX_DATA || frame.dlc == 0)
        {
            CanTsHeader nack{};
            nack.to = h.from;
            nack.from = localAddress;
            nack.type = CanTsMessageType::GETBLOCK;
            nack.command = makeGetBlockCommand(GetBlockFrameType::NACK, ackCommandLow7(h.command));
            (void)sendFrame(nack, frame.data, frame.dlc);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(mtx);

            BlockSession session{};
            session.peer = h.from;
            session.channel = 0;
            session.address.assign(frame.data, frame.data + frame.dlc);
            session.numBlocks = numBlocks;
            session.blocks.resize(numBlocks);
            session.lastActivity = std::chrono::steady_clock::now();
            session.prepared = true;
            session.accepted = true;

            for (uint8_t i = 0; i < numBlocks; ++i)
            {
                session.blocks[i].assign(responseData.begin() + (i * CAN_FRAME_MAX_DATA),
                                         responseData.begin() + ((i + 1) * CAN_FRAME_MAX_DATA));
            }

            gbSessions[h.from] = std::move(session);
        }

        CanTsHeader ack{};
        ack.to = h.from;
        ack.from = localAddress;
        ack.type = CanTsMessageType::GETBLOCK;
        ack.command = makeGetBlockCommand(GetBlockFrameType::ACK, ackCommandLow7(h.command));

        (void)sendFrame(ack, frame.data, frame.dlc);
        break;
    }

    case GetBlockFrameType::START: {
        std::vector<std::vector<uint8_t>> blocksToSend;
        uint8_t numBlocks = 0;
        bool valid = false;

        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = gbSessions.find(h.from);
            if (it != gbSessions.end() && it->second.accepted)
            {
                const std::vector<uint8_t> bitmap(frame.data, frame.data + frame.dlc);
                it->second.lastActivity = std::chrono::steady_clock::now();

                numBlocks = it->second.numBlocks;
                blocksToSend = it->second.blocks;
                valid = true;

                for (uint8_t i = 0; i < numBlocks; ++i)
                {
                    if (!bitmapBitIsSet(bitmap, i))
                    {
                        blocksToSend[i].clear();
                    }
                }
            }
        }

        if (!valid)
        {
            CanTsHeader nack{};
            nack.to = h.from;
            nack.from = localAddress;
            nack.type = CanTsMessageType::GETBLOCK;
            nack.command = makeGetBlockCommand(GetBlockFrameType::NACK, ackCommandLow7(h.command));
            (void)sendFrame(nack, frame.data, frame.dlc);
            return;
        }

        for (uint8_t i = 0; i < numBlocks; ++i)
        {
            if (blocksToSend[i].empty())
            {
                continue;
            }

            CanTsHeader transfer{};
            transfer.to = h.from;
            transfer.from = localAddress;
            transfer.type = CanTsMessageType::GETBLOCK;
            transfer.command = makeGetBlockCommand(GetBlockFrameType::TRANSFER, i);

            const uint8_t dlc = static_cast<uint8_t>(std::min<size_t>(blocksToSend[i].size(), CAN_FRAME_MAX_DATA));
            (void)sendFrame(transfer, blocksToSend[i].data(), dlc);
        }
        break;
    }

    case GetBlockFrameType::ABORT: {
        std::lock_guard<std::mutex> lock(mtx);
        gbSessions.erase(h.from);
        break;
    }

    case GetBlockFrameType::ACK: {
        std::lock_guard<std::mutex> lock(mtx);
        if (pendingGb.waitingAck && pendingGb.peer == h.from)
        {
            pendingGb.acked = true;
            pendingGb.waitingAck = false;
            cv.notify_all();
        }
        break;
    }

    case GetBlockFrameType::NACK: {
        std::lock_guard<std::mutex> lock(mtx);
        if ((pendingGb.waitingAck || pendingGb.waitingTransfers) && pendingGb.peer == h.from)
        {
            pendingGb.done = true;
            pendingGb.ok = false;
            cv.notify_all();
        }
        break;
    }

    case GetBlockFrameType::TRANSFER: {
        std::lock_guard<std::mutex> lock(mtx);
        if (!pendingGb.waitingTransfers || pendingGb.peer != h.from)
        {
            return;
        }

        const uint8_t seq = getGetBlockLow6(h.command);
        if (seq >= pendingGb.numBlocks)
        {
            return;
        }

        pendingGb.blocks[seq].assign(frame.data, frame.data + frame.dlc);
        pendingGb.received[seq] = true;

        if (allBlocksReceived(pendingGb.received))
        {
            pendingGb.done = true;
            pendingGb.ok = true;

            lastGetBlockData.clear();
            for (uint8_t i = 0; i < pendingGb.numBlocks; ++i)
            {
                lastGetBlockData.insert(lastGetBlockData.end(), pendingGb.blocks[i].begin(), pendingGb.blocks[i].end());
            }

            cv.notify_all();
        }
        break;
    }

    default:
        break;
    }
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

    const CanTsHeader h{to, localAddress, CanTsMessageType::TELECOMMAND, makeTcTmCommand(CanTsReqAck::REQ, channel)};

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

    const CanTsHeader h{to, localAddress, CanTsMessageType::TELEMETRY, makeTcTmCommand(CanTsReqAck::REQ, channel)};

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
    if (addressLE.empty() || data.empty() || data.size() > CAN_TS_MAX_BLOCK_BYTES)
    {
        return false;
    }

    const size_t numBlocksSz = (data.size() + CAN_FRAME_MAX_DATA - 1u) / CAN_FRAME_MAX_DATA;
    if (numBlocksSz == 0 || numBlocksSz > 64)
    {
        return false;
    }

    const uint8_t numBlocks = static_cast<uint8_t>(numBlocksSz);
    std::vector<std::vector<uint8_t>> blocks(numBlocks, std::vector<uint8_t>(CAN_FRAME_MAX_DATA, 0));

    for (uint8_t i = 0; i < numBlocks; ++i)
    {
        const size_t start = static_cast<size_t>(i) * CAN_FRAME_MAX_DATA;
        const size_t len = std::min<size_t>(CAN_FRAME_MAX_DATA, data.size() - start);
        std::copy_n(data.begin() + start, len, blocks[i].begin());
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        pendingSb = {};
        pendingSb.active = true;
        pendingSb.waitingRequestAck = true;
        pendingSb.peer = to;
        pendingSb.numBlocks = numBlocks;
    }

    CanTsHeader req{};
    req.to = to;
    req.from = localAddress;
    req.type = CanTsMessageType::SETBLOCK;
    req.command = makeSetBlockCommand(SetBlockFrameType::REQUEST, static_cast<uint8_t>(numBlocks - 1u));

    if (!sendFrame(req, addressLE.data(), static_cast<uint8_t>(std::min<size_t>(addressLE.size(), CAN_FRAME_MAX_DATA))))
    {
        std::lock_guard<std::mutex> lock(mtx);
        pendingSb = {};
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(mtx);
        const bool gotRequestAck = cv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                               [&] { return pendingSb.requestAcked || pendingSb.done; });

        if (!gotRequestAck || pendingSb.done || !pendingSb.requestAcked)
        {
            pendingSb = {};
            return false;
        }
    }

    for (uint8_t i = 0; i < numBlocks; ++i)
    {
        CanTsHeader tr{};
        tr.to = to;
        tr.from = localAddress;
        tr.type = CanTsMessageType::SETBLOCK;
        tr.command = makeSetBlockCommand(SetBlockFrameType::TRANSFER, i);

        if (!sendFrame(tr, blocks[i].data(), CAN_FRAME_MAX_DATA))
        {
            std::lock_guard<std::mutex> lock(mtx);
            pendingSb = {};
            return false;
        }
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        pendingSb.waitingReport = true;
        pendingSb.reportReceived = false;
        pendingSb.done = false;
        pendingSb.ok = false;
    }

    CanTsHeader statusReq{};
    statusReq.to = to;
    statusReq.from = localAddress;
    statusReq.type = CanTsMessageType::SETBLOCK;
    statusReq.command = makeSetBlockCommand(SetBlockFrameType::STATUS_REQUEST, 0);

    if (!sendFrame(statusReq, nullptr, 0))
    {
        std::lock_guard<std::mutex> lock(mtx);
        pendingSb = {};
        return false;
    }

    bool success = false;
    {
        std::unique_lock<std::mutex> lock(mtx);
        const bool gotReport = cv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                           [&] { return pendingSb.reportReceived || pendingSb.done; });

        if (!gotReport || !pendingSb.ok || !pendingSb.reportReceived)
        {
            pendingSb = {};
            return false;
        }

        bool allSet = true;
        for (uint8_t i = 0; i < numBlocks; ++i)
        {
            if (!bitmapBitIsSet(pendingSb.bitmap, i))
            {
                allSet = false;
                break;
            }
        }

        success = allSet && pendingSb.done;
        pendingSb = {};
    }

    CanTsHeader abort{};
    abort.to = to;
    abort.from = localAddress;
    abort.type = CanTsMessageType::SETBLOCK;
    abort.command = makeSetBlockCommand(SetBlockFrameType::ABORT, 0);
    (void)sendFrame(abort, nullptr, 0);

    return success;
}

bool CanTsProtocol::getBlock(uint8_t to, const std::vector<uint8_t> &addressLE, uint32_t timeoutMs)
{
    if (addressLE.empty())
    {
        return false;
    }

    // This still assumes a fixed 64-block transfer shape.
    // If you want a general solution, the protocol needs a size negotiation step.
    const uint8_t numBlocks = 64;

    {
        std::lock_guard<std::mutex> lock(mtx);
        pendingGb = {};
        pendingGb.waitingAck = true;
        pendingGb.peer = to;
        pendingGb.numBlocks = numBlocks;
    }

    CanTsHeader req{};
    req.to = to;
    req.from = localAddress;
    req.type = CanTsMessageType::GETBLOCK;
    req.command = makeGetBlockCommand(GetBlockFrameType::REQUEST, static_cast<uint8_t>(numBlocks - 1u));

    if (!sendFrame(req, addressLE.data(), static_cast<uint8_t>(std::min<size_t>(addressLE.size(), CAN_FRAME_MAX_DATA))))
    {
        std::lock_guard<std::mutex> lock(mtx);
        pendingGb = {};
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(mtx);
        const bool acked =
            cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] { return pendingGb.acked || pendingGb.done; });

        if (!acked || pendingGb.done)
        {
            pendingGb = {};
            return false;
        }

        pendingGb.waitingTransfers = true;
        pendingGb.blocks.assign(numBlocks, {});
        pendingGb.received.assign(numBlocks, false);
    }

    std::vector<uint8_t> bitmap(bitmapSizeBytes(numBlocks), 0xFFu);
    if (!bitmap.empty())
    {
        const uint8_t extraBits = static_cast<uint8_t>((bitmap.size() * 8u) - numBlocks);
        if (extraBits > 0)
        {
            bitmap.back() &= static_cast<uint8_t>(0xFFu >> extraBits);
        }
    }

    CanTsHeader start{};
    start.to = to;
    start.from = localAddress;
    start.type = CanTsMessageType::GETBLOCK;
    start.command = makeGetBlockCommand(GetBlockFrameType::START, 0);

    if (!sendFrame(start, bitmap.data(), static_cast<uint8_t>(std::min<size_t>(bitmap.size(), CAN_FRAME_MAX_DATA))))
    {
        std::lock_guard<std::mutex> lock(mtx);
        pendingGb = {};
        return false;
    }

    bool success = false;
    {
        std::unique_lock<std::mutex> lock(mtx);
        const bool done = cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] { return pendingGb.done; });
        success = done && pendingGb.ok;
        pendingGb = {};
    }

    return success;
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

bool CanTsProtocol::getLastGetBlockData(std::vector<uint8_t> &out) const
{
    std::lock_guard<std::mutex> lock(mtx);
    if (lastGetBlockData.empty())
    {
        return false;
    }

    out = lastGetBlockData;
    return true;
}

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
