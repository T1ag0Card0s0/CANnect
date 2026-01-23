#include "cannect/core/cants/CanTsProtocol.hpp"

#include <chrono>
#include <cstring>
#include <thread>

#include "cannect/core/cants/EncoderDecoder.hpp"

namespace cannect
{

using namespace cannect::cants;

static std::vector<uint8_t> frameDataToVec(const CanFrame &f)
{
    return std::vector<uint8_t>(f.getData(), f.getData() + f.getDLC());
}

CanTsProtocol::CanTsProtocol(ICanTransport &transport, uint8_t localAddress)
    : sender(transport), local(localAddress)
{
}

void CanTsProtocol::setTelecommandHandler(TcHandler h)
{
    std::lock_guard<std::mutex> lock(mtx);
    tcHandler = std::move(h);
}

void CanTsProtocol::setTelemetryHandler(TmHandler h)
{
    std::lock_guard<std::mutex> lock(mtx);
    tmHandler = std::move(h);
}

void CanTsProtocol::setUnsolicitedTelemetryHandler(UTMHandler h)
{
    std::lock_guard<std::mutex> lock(mtx);
    utmHandler = std::move(h);
}

void CanTsProtocol::setTimeSyncHandler(TimeSyncHandler h)
{
    std::lock_guard<std::mutex> lock(mtx);
    tsHandler = std::move(h);
}

bool CanTsProtocol::sendFrame(uint8_t to, CanTsMessageType type, uint16_t command, const uint8_t *data, uint8_t len)
{
    CanTsHeader h{};
    h.to = to;
    h.from = local;
    h.type = type;
    h.command = static_cast<uint16_t>(command & CMD_MASK_10BIT);

    uint32_t id = EncoderDecoder::encode(h);

    CanFrame f(id, data, len);
    return sender.sendFrame(f) > 0;
}

void CanTsProtocol::update(const CanFrame &frame)
{
    CanTsHeader h = EncoderDecoder::decode(frame.getCanId());

    // Accept if addressed to us, or broadcast (To=0) for time sync. :contentReference[oaicite:11]{index=11}
    if (h.to != local && h.to != 0)
    {
        return;
    }

    switch (h.type)
    {
        case CanTsMessageType::TELECOMMAND:
            handleTelecommand(h, frame);
            break;
        case CanTsMessageType::TELEMETRY:
            handleTelemetry(h, frame);
            break;
        case CanTsMessageType::UNSOLICITED:
            handleUnsolicited(h, frame);
            break;
        case CanTsMessageType::TIMESYNC:
            handleTimeSync(h, frame);
            break;
        case CanTsMessageType::SETBLOCK:
            handleSetBlock(h, frame);
            break;
        case CanTsMessageType::GETBLOCK:
            handleGetBlock(h, frame);
            break;
        default:
            break;
    }
}

void CanTsProtocol::handleTelecommand(const CanTsHeader &h, const CanFrame &frame)
{
    CanTsReqAck ra = tc_tm_reqack(h.command);
    uint8_t channel = tc_tm_channel(h.command);

    if (ra != CanTsReqAck::REQ)
    {
        // Source-side code can process ACK/NACK if you extend receive(); for now ignore.
        return;
    }

    std::vector<uint8_t> args = frameDataToVec(frame);
    std::vector<uint8_t> resp;

    TcHandler handlerCopy;
    {
        std::lock_guard<std::mutex> lock(mtx);
        handlerCopy = tcHandler;
    }

    bool ok = false;
    if (handlerCopy)
    {
        ok = handlerCopy(h.from, channel, args, resp);
    }

    // ACK/NACK response is addressed back to requester and uses same channel; bits 9-8 indicate ACK/NACK. :contentReference[oaicite:12]{index=12}
    uint16_t cmd = make_tc_tm_cmd(ok ? CanTsReqAck::ACK : CanTsReqAck::NACK, channel);

    if (resp.size() > CANTS_FRAME_MAX_DATA_SIZE)
    {
        resp.resize(CANTS_FRAME_MAX_DATA_SIZE);
    }

    sendFrame(h.from, CanTsMessageType::TELECOMMAND, cmd, resp.data(),
              static_cast<uint8_t>(resp.size()));
}

void CanTsProtocol::handleTelemetry(const CanTsHeader &h, const CanFrame &frame)
{
    CanTsReqAck ra = tc_tm_reqack(h.command);
    uint8_t channel = tc_tm_channel(h.command);

    if (ra == CanTsReqAck::REQ)
    {
        // We are the sink; respond with value or NACK. :contentReference[oaicite:13]{index=13}
        std::vector<uint8_t> value;

        TmHandler handlerCopy;
        {
            std::lock_guard<std::mutex> lock(mtx);
            handlerCopy = tmHandler;
        }

        bool ok = false;
        if (handlerCopy)
        {
            ok = handlerCopy(h.from, channel, value);
        }

        if (value.size() > CANTS_FRAME_MAX_DATA_SIZE)
        {
            value.resize(CANTS_FRAME_MAX_DATA_SIZE);
        }

        uint16_t cmd = make_tc_tm_cmd(ok ? CanTsReqAck::ACK : CanTsReqAck::NACK, channel);
        sendFrame(h.from, CanTsMessageType::TELEMETRY, cmd, value.data(), static_cast<uint8_t>(value.size()));
        return;
    }

    // Source-side ACK/NACK received. You can deliver it via receive() later if needed.
    (void)frame;
}

void CanTsProtocol::handleUnsolicited(const CanTsHeader &h, const CanFrame &frame)
{
    uint8_t channel = tc_tm_channel(h.command);

    UTMHandler handlerCopy;
    {
        std::lock_guard<std::mutex> lock(mtx);
        handlerCopy = utmHandler;
    }

    if (handlerCopy)
    {
        handlerCopy(h.from, channel, frameDataToVec(frame));
    }
}

void CanTsProtocol::handleTimeSync(const CanTsHeader &h, const CanFrame &frame)
{
    TimeSyncHandler handlerCopy;
    {
        std::lock_guard<std::mutex> lock(mtx);
        handlerCopy = tsHandler;
    }

    if (handlerCopy)
    {
        handlerCopy(h.from, frameDataToVec(frame));
    }
}

static void setBitmapBit(std::vector<uint8_t> &bmp, uint8_t idx)
{
    uint8_t byte = idx / 8;
    uint8_t bit = idx % 8;
    if (byte >= bmp.size())
    {
        bmp.resize(byte + 1, 0);
    }
    bmp[byte] |= static_cast<uint8_t>(1u << bit);
}

static bool getBitmapBit(const std::vector<uint8_t> &bmp, uint8_t idx)
{
    uint8_t byte = idx / 8;
    uint8_t bit = idx % 8;
    if (byte >= bmp.size())
    {
        return false;
    }
    return ((bmp[byte] >> bit) & 1u) != 0;
}

void CanTsProtocol::handleSetBlock(const CanTsHeader &h, const CanFrame &frame)
{
    uint8_t t = sb_gb_type3(h.command);
    auto ft = static_cast<SetBlockFrameType>(t);

    std::lock_guard<std::mutex> lock(mtx);

    if (ft == SetBlockFrameType::REQUEST)
    {
        // Request data contains start address (LE, 1..max bytes, system-specific). :contentReference[oaicite:14]{index=14}
        // Number of blocks = (bits5-0)+1. :contentReference[oaicite:15]{index=15}
        uint8_t blocksMinus1 = sb_gb_index6(h.command);
        uint8_t numBlocks = static_cast<uint8_t>(blocksMinus1 + 1);

        SetBlockSession s{};
        s.address = frameDataToVec(frame);
        s.numBlocks = numBlocks;
        s.blocks.resize(numBlocks);
        s.bitmap.resize((numBlocks + 7) / 8, 0);
        s.done = false;

        sbSessions[h.from] = std::move(s);

        // ACK should contain copy of data and command field (bit6-0) from frame it acknowledges. :contentReference[oaicite:16]{index=16}
        // For simplicity we echo address bytes as "copy of data".
        uint16_t ackCmd = make_sb_cmd(SetBlockFrameType::ACK, (h.command & 0x7F));
        auto dataVec = frameDataToVec(frame);
        if (dataVec.size() > CANTS_FRAME_MAX_DATA_SIZE)
        {
            dataVec.resize(CANTS_FRAME_MAX_DATA_SIZE);
        }
        sendFrame(h.from, CanTsMessageType::SETBLOCK, ackCmd, dataVec.data(), static_cast<uint8_t>(dataVec.size()));
        return;
    }

    auto it = sbSessions.find(h.from);
    if (it == sbSessions.end())
    {
        return;
    }

    SetBlockSession &s = it->second;

    if (ft == SetBlockFrameType::TRANSFER)
    {
        uint8_t seq = sb_gb_index6(h.command);
        if (seq < s.numBlocks)
        {
            s.blocks[seq] = frameDataToVec(frame);
            setBitmapBit(s.bitmap, seq);

            // If all bits are set => received all blocks.
            bool all = true;
            for (uint8_t i = 0; i < s.numBlocks; ++i)
            {
                if (!getBitmapBit(s.bitmap, i))
                {
                    all = false;
                    break;
                }
            }
            if (all)
            {
                // Mark done = true (processing could be async in real system). :contentReference[oaicite:17]{index=17}
                s.done = true;
            }
        }
        return;
    }

    if (ft == SetBlockFrameType::STATUS_REQUEST)
    {
        // Reply with REPORT containing bitmap; Done bit in cmd bit6. :contentReference[oaicite:18]{index=18}
        uint16_t reportCmd = make_sb_cmd(SetBlockFrameType::REPORT, 0, s.done);

        std::vector<uint8_t> bmp = s.bitmap;
        if (bmp.size() > CANTS_FRAME_MAX_DATA_SIZE)
        {
            bmp.resize(CANTS_FRAME_MAX_DATA_SIZE);
        }
        sendFrame(h.from, CanTsMessageType::SETBLOCK, reportCmd, bmp.data(), static_cast<uint8_t>(bmp.size()));
        return;
    }

    if (ft == SetBlockFrameType::ABORT)
    {
        // Abort ends session. Sink confirms with SB ACK. :contentReference[oaicite:19]{index=19}
        uint16_t ackCmd = make_sb_cmd(SetBlockFrameType::ACK, (h.command & 0x7F));
        uint8_t dummy = 0;
        sendFrame(h.from, CanTsMessageType::SETBLOCK, ackCmd, &dummy, 0);
        sbSessions.erase(it);
        return;
    }
}

void CanTsProtocol::handleGetBlock(const CanTsHeader &h, const CanFrame &frame)
{
    uint8_t t = sb_gb_type3(h.command);
    auto ft = static_cast<GetBlockFrameType>(t);

    std::lock_guard<std::mutex> lock(mtx);

    if (ft == GetBlockFrameType::REQUEST)
    {
        // Request contains address (LE). Num blocks = (bits5-0)+1. :contentReference[oaicite:20]{index=20}
        uint8_t blocksMinus1 = sb_gb_index6(h.command);
        uint8_t numBlocks = static_cast<uint8_t>(blocksMinus1 + 1);

        GetBlockSession s{};
        s.address = frameDataToVec(frame);
        s.numBlocks = numBlocks;
        s.blocks.resize(numBlocks);

        gbSessions[h.from] = std::move(s);

        // ACK echoes copy of data + command bits6-0 of request. :contentReference[oaicite:21]{index=21}
        uint16_t ackCmd = make_gb_cmd(GetBlockFrameType::ACK, (h.command & 0x7F));
        auto dataVec = frameDataToVec(frame);
        if (dataVec.size() > CANTS_FRAME_MAX_DATA_SIZE)
        {
            dataVec.resize(CANTS_FRAME_MAX_DATA_SIZE);
        }
        sendFrame(h.from, CanTsMessageType::GETBLOCK, ackCmd, dataVec.data(), static_cast<uint8_t>(dataVec.size()));
        return;
    }

    // For simplicity, this implementation is "sink-only" unless you call getBlock() which drives START.
    // If you later want to be a full sink that can send TRANSFER frames on START,
    // you need a data-provider callback based on address.
    (void)frame;
}

void CanTsProtocol::send(const std::vector<uint8_t> &data)
{
    // Not meaningful for CAN-TS without metadata; keep for interface compatibility.
    (void)data;
}

std::vector<uint8_t> CanTsProtocol::receive()
{
    // For now unused; you can extend this with a queue of received ACK/TM values.
    return {};
}

bool CanTsProtocol::sendTelecommand(uint8_t to, uint8_t channel, const std::vector<uint8_t> &args)
{
    std::vector<uint8_t> d = args;
    if (d.size() > CANTS_FRAME_MAX_DATA_SIZE)
    {
        d.resize(CANTS_FRAME_MAX_DATA_SIZE);
    }
    return sendFrame(to, CanTsMessageType::TELECOMMAND, make_tc_tm_cmd(CanTsReqAck::REQ, channel), d.data(),
                     static_cast<uint8_t>(d.size()));
}

bool CanTsProtocol::requestTelemetry(uint8_t to, uint8_t channel)
{
    return sendFrame(to, CanTsMessageType::TELEMETRY, make_tc_tm_cmd(CanTsReqAck::REQ, channel), nullptr, 0);
}

bool CanTsProtocol::sendUnsolicitedTelemetry(uint8_t to, uint8_t channel, const std::vector<uint8_t> &value)
{
    std::vector<uint8_t> d = value;
    if (d.size() > CANTS_FRAME_MAX_DATA_SIZE)
    {
        d.resize(CANTS_FRAME_MAX_DATA_SIZE);
    }
    // Command bits7-0 = channel; bits9-8 unused here in spec's UTM description. :contentReference[oaicite:22]{index=22}
    uint16_t cmd = channel;
    return sendFrame(to, CanTsMessageType::UNSOLICITED, cmd, d.data(), static_cast<uint8_t>(d.size()));
}

bool CanTsProtocol::broadcastTimeSync(const std::vector<uint8_t> &timeLE)
{
    std::vector<uint8_t> d = timeLE;
    if (d.size() > CANTS_FRAME_MAX_DATA_SIZE)
    {
        d.resize(CANTS_FRAME_MAX_DATA_SIZE);
    }
    // To=0 broadcast for time sync. :contentReference[oaicite:23]{index=23}
    return sendFrame(0, CanTsMessageType::TIMESYNC, 0, d.data(), static_cast<uint8_t>(d.size()));
}

bool CanTsProtocol::setBlock(uint8_t to, const std::vector<uint8_t> &addressLE, const std::vector<uint8_t> &data,
                             uint32_t timeoutMs)
{
    // Source-side basic: send REQUEST then TRANSFER frames; polling for status/bitmap not implemented yet.
    // Spec: blocks are 8 bytes chunks; last may be shorter. Up to 512 bytes. :contentReference[oaicite:24]{index=24}
    uint8_t numBlocks = static_cast<uint8_t>((data.size() + 7) / 8);
    if (numBlocks == 0 || numBlocks > 64)
    {
        return false;
    }

    uint16_t reqCmd = make_sb_cmd(SetBlockFrameType::REQUEST, static_cast<uint8_t>(numBlocks - 1));
    if (!sendFrame(to, CanTsMessageType::SETBLOCK, reqCmd, addressLE.data(),
                   static_cast<uint8_t>(std::min<size_t>(addressLE.size(), CANTS_FRAME_MAX_DATA_SIZE))))
    {
        return false;
    }

    // Give receiver a brief moment (no ACK tracking in this minimal build).
    std::this_thread::sleep_for(std::chrono::milliseconds(timeoutMs));

    for (uint8_t i = 0; i < numBlocks; ++i)
    {
        size_t off = static_cast<size_t>(i) * 8;
        size_t len = std::min<size_t>(8, data.size() - off);

        uint16_t trCmd = make_sb_cmd(SetBlockFrameType::TRANSFER, i);
        if (!sendFrame(to, CanTsMessageType::SETBLOCK, trCmd, data.data() + off, static_cast<uint8_t>(len)))
        {
            return false;
        }
    }

    return true;
}

bool CanTsProtocol::getBlock(uint8_t to, const std::vector<uint8_t> &addressLE, uint8_t numBlocks,
                             std::vector<uint8_t> &out, uint32_t timeoutMs)
{
    // Minimal: send REQUEST and START bitmap "all blocks"; receiving TRANSFER frames is not implemented yet.
    // You can extend handleGetBlock() to process TRANSFER and fill a queue. :contentReference[oaicite:25]{index=25}
    if (numBlocks == 0 || numBlocks > 64)
    {
        return false;
    }

    uint16_t reqCmd = make_gb_cmd(GetBlockFrameType::REQUEST, static_cast<uint8_t>(numBlocks - 1));
    if (!sendFrame(to, CanTsMessageType::GETBLOCK, reqCmd, addressLE.data(),
                   static_cast<uint8_t>(std::min<size_t>(addressLE.size(), CANTS_FRAME_MAX_DATA_SIZE))))
    {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(timeoutMs));

    // Bitmap length = ceil(numBlocks/8), little endian, unused bits 0. :contentReference[oaicite:26]{index=26}
    std::vector<uint8_t> bmp((numBlocks + 7) / 8, 0);
    for (uint8_t i = 0; i < numBlocks; ++i)
    {
        setBitmapBit(bmp, i);
    }
    if (bmp.size() > CANTS_FRAME_MAX_DATA_SIZE)
    {
        bmp.resize(CANTS_FRAME_MAX_DATA_SIZE);
    }

    uint16_t startCmd = make_gb_cmd(GetBlockFrameType::START, 0);
    if (!sendFrame(to, CanTsMessageType::GETBLOCK, startCmd, bmp.data(), static_cast<uint8_t>(bmp.size())))
    {
        return false;
    }

    // Not implemented: collect transfer frames and assemble out.
    out.clear();
    return true;
}

} // namespace cannect
