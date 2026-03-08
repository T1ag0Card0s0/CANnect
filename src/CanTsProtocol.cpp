#include "CanTsProtocol.hpp"

using namespace cannect;

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
Status CanTsProtocol::setFrameTransmitter(std::shared_ptr<ICanFrameTransmitter> transmitter)
{
    this->transmitter = std::move(transmitter);
    return Status::SUCCESS;
}

Status CanTsProtocol::onFrame(const CanFrame &frame)
{
    CanTsHeader header = EncoderDecoder::decode(frame.id);

    if (header.to != localAddress && header.to != 0)
    {
        return Status::UNSUCCESS;
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

void CanTsProtocol::handleTelecommand(const CanTsHeader &h, const CanFrame &frame) {}

void CanTsProtocol::handleTelemetry(const CanTsHeader &h, const CanFrame &frame) {}

void CanTsProtocol::handleUnsolicited(const CanTsHeader &h, const CanFrame &frame) {}

void CanTsProtocol::handleTimeSync(const CanTsHeader &h, const CanFrame &frame) {}

void CanTsProtocol::handleSetBlock(const CanTsHeader &h, const CanFrame &frame) {}

void CanTsProtocol::handleGetBlock(const CanTsHeader &h, const CanFrame &frame) {}

bool CanTsProtocol::sendTelecommand(uint8_t to, uint8_t channel, const uint8_t data[CAN_FRAME_MAX_DATA]) {}

bool CanTsProtocol::requestTelemetry(uint8_t to, uint8_t channel) {}

bool CanTsProtocol::sendUnsolicitedTelemetry(uint8_t to, uint8_t channel, const uint8_t data[CAN_FRAME_MAX_DATA]) {}

bool CanTsProtocol::broadcastTimeSync(const uint8_t timeLE[CAN_FRAME_MAX_DATA]) {}

bool CanTsProtocol::setBlock(uint8_t to, const std::vector<uint8_t> &addressLE, const std::vector<uint8_t> &data,
                             uint32_t timeoutMs = DEFAULT_TIMEOUT_MS)
{
}

bool CanTsProtocol::getBlock(uint8_t to, const std::vector<uint8_t> &addressLE, uint32_t timeoutMs = DEFAULT_TIMEOUT_MS)
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

void CanTsProtocol::setGetBlockHandler(GetBlockHandler h)
{
    std::lock_guard<std::mutex> lock(mtx);
    getBlockHandler = std::move(h);
}

void CanTsProtocol::setSetBlockHandler(SetBlockHandler h)
{
    std::lock_guard<std::mutex> lock(mtx);
    setBlockHandler = std::move(h);
}
