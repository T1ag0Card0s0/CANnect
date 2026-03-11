#pragma once

#include "cannect/ICanProtocol.hpp"
#include "cannect/Status.hpp"
#include "cannect/Types.hpp"

#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>

#define DEFAULT_TIMEOUT_MS 200

namespace cannect
{
enum class CanTsMessageType : uint8_t
{
    TIMESYNC = 0, // 000
    UNSOLICITED = 1, // 001
    TELECOMMAND = 2, // 010
    TELEMETRY = 3, // 011
    SETBLOCK = 4, // 100
    GETBLOCK = 5 // 101
};

enum class CanTsReqAck : uint8_t
{
    REQ = 0b00,
    ACK = 0b01,
    NACK = 0b10
};

enum class SetBlockFrameType : uint8_t
{
    REQUEST = 0b000,
    TRANSFER = 0b001,
    ACK = 0b010,
    ABORT = 0b011,
    NACK = 0b100,
    STATUS_REQUEST = 0b110,
    REPORT = 0b111
};

enum class GetBlockFrameType : uint8_t
{
    REQUEST = 0b000,
    ACK = 0b010,
    ABORT = 0b011,
    NACK = 0b100,
    START = 0b110,
    TRANSFER = 0b111
};

struct __attribute__((packed)) CanTsHeader
{
    uint8_t to;
    uint8_t from;
    CanTsMessageType type;
    uint16_t command;
};

class CanTsProtocol : public ICanProtocol
{
  public:
    CanTsProtocol(uint8_t localAddress);

    using TcHandler = std::function<bool(uint8_t from, uint8_t channel, uint8_t request[CAN_FRAME_MAX_DATA])>;
    using TmHandler = std::function<bool(uint8_t from, uint8_t channel, uint8_t response[CAN_FRAME_MAX_DATA])>;
    using UTMHandler = std::function<void(uint8_t from, uint8_t channel, uint8_t data[CAN_FRAME_MAX_DATA])>;
    using TimeSyncHandler = std::function<void(uint8_t from, uint8_t timeLE[CAN_FRAME_MAX_DATA])>;
    using SetBlockHandler = std::function<bool(uint8_t from, uint8_t channel, std::vector<uint8_t> &request)>;
    using GetBlockHandler = std::function<bool(uint8_t from, uint8_t channel, std::vector<uint8_t> &response)>;

    Status onFrame(const CanFrame &frame) override;
    Status setFrameTransmitter(std::shared_ptr<ICanFrameTransmitter> transmitter) override;

    void setTelecommandHandler(TcHandler handler);
    void setTelemetryHandler(TmHandler handler);
    void setUnsolicitedTelemetryHandler(UTMHandler handler);
    void setTimeSyncHandler(TimeSyncHandler handler);
    void setSetBlockHandler(SetBlockHandler handler);
    void setGetBlockHandler(GetBlockHandler handler);

    bool sendTelecommand(uint8_t to, uint8_t channel, const uint8_t data[CAN_FRAME_MAX_DATA]);
    bool requestTelemetry(uint8_t to, uint8_t channel);
    bool sendUnsolicitedTelemetry(uint8_t to, uint8_t channel, const uint8_t data[CAN_FRAME_MAX_DATA]);
    bool broadcastTimeSync(const uint8_t timeLE[CAN_FRAME_MAX_DATA]);
    bool setBlock(uint8_t to, const std::vector<uint8_t> &addressLE, const std::vector<uint8_t> &data,
                  uint32_t timeoutMs = DEFAULT_TIMEOUT_MS);
    bool getBlock(uint8_t to, const std::vector<uint8_t> &addressLE, uint32_t timeoutMs = DEFAULT_TIMEOUT_MS);

  private:
    void sendNack(CanTsHeader &h, const CanFrame &frame);
    void handleTelecommand(const CanTsHeader &h, const CanFrame &frame);
    void handleTelemetry(const CanTsHeader &h, const CanFrame &frame);
    void handleUnsolicited(const CanTsHeader &h, const CanFrame &frame);
    void handleTimeSync(const CanTsHeader &h, const CanFrame &frame);
    void handleSetBlock(const CanTsHeader &h, const CanFrame &frame);
    void handleGetBlock(const CanTsHeader &h, const CanFrame &frame);

    struct BlockSession
    {
        std::vector<uint8_t> address;
        uint8_t numBlocks = 0;
        std::vector<std::vector<uint8_t>> blocks;
        std::vector<std::vector<uint8_t>> messages;
    };

    TcHandler tcHandler;
    TmHandler tmHandler;
    UTMHandler utmHandler;
    TimeSyncHandler tsHandler;
    SetBlockHandler setBlockHandler;
    GetBlockHandler getBlockHandler;

    std::unordered_map<uint8_t, BlockSession> sbSessions;
    std::unordered_map<uint8_t, BlockSession> gbSessions;

    std::shared_ptr<ICanFrameTransmitter> transmitter;

    uint8_t localAddress;
    std::mutex mtx;
};

} // namespace cannect
