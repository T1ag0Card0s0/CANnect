#pragma once

#include "cannect/ICanProtocol.hpp"
#include "cannect/Status.hpp"
#include "cannect/Types.hpp"

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

#define DEFAULT_TIMEOUT_MS 200
#define CAN_TS_MAX_BLOCK_BYTES 512
#define CAN_TS_KEEPALIVE_ADDRESS 1

namespace cannect
{

enum class CanTsMessageType : uint8_t
{
    TIMESYNC = 0,
    UNSOLICITED = 1,
    TELECOMMAND = 2,
    TELEMETRY = 3,
    SETBLOCK = 4,
    GETBLOCK = 5
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
    uint8_t to = 0;
    uint8_t from = 0;
    CanTsMessageType type = CanTsMessageType::TIMESYNC;
    uint16_t command = 0;
};

class CanTsProtocol : public ICanProtocol
{
  public:
    explicit CanTsProtocol(uint8_t localAddress);

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

    bool getLastTelemetry(uint8_t out[CAN_FRAME_MAX_DATA], uint8_t &dlc) const;
    bool getLastGetBlockData(std::vector<uint8_t> &out) const;

    bool isPeerConnected(uint8_t peer, uint32_t timeoutMs) const;
    bool sendKeepAlive(uint8_t channel, const uint8_t data[CAN_FRAME_MAX_DATA]);

  private:
    struct BlockSession
    {
        uint8_t peer = 0;
        uint8_t channel = 0;
        std::vector<uint8_t> address;
        uint8_t numBlocks = 0;
        std::vector<std::vector<uint8_t>> blocks;
        std::chrono::steady_clock::time_point lastActivity{};
        bool prepared = false;
        bool done = false;
        bool accepted = false;
    };

    struct PendingTcTm
    {
        bool waiting = false;
        bool done = false;
        bool ok = false;
        uint8_t peer = 0;
        uint8_t channel = 0;
        uint8_t data[CAN_FRAME_MAX_DATA] = {};
        uint8_t dlc = 0;
    };

    struct PendingSetBlock
    {
        bool active = false;
        bool waitingRequestAck = false;
        bool requestAcked = false;
        bool waitingReport = false;
        bool reportReceived = false;
        bool done = false;
        bool ok = false;
        uint8_t peer = 0;
        uint8_t channel = 0;
        uint8_t numBlocks = 0;
        std::vector<uint8_t> bitmap;
    };

    struct PendingGetBlock
    {
        bool waitingAck = false;
        bool acked = false;
        bool waitingTransfers = false;
        bool done = false;
        bool ok = false;
        uint8_t peer = 0;
        uint8_t channel = 0;
        uint8_t numBlocks = 0;
        std::vector<std::vector<uint8_t>> blocks;
        std::vector<bool> received;
    };

    struct PeerState
    {
        std::chrono::steady_clock::time_point lastSeen{};
    };

    void handleTelecommand(const CanTsHeader &h, const CanFrame &frame);
    void handleTelemetry(const CanTsHeader &h, const CanFrame &frame);
    void handleUnsolicited(const CanTsHeader &h, const CanFrame &frame);
    void handleTimeSync(const CanTsHeader &h, const CanFrame &frame);
    void handleSetBlock(const CanTsHeader &h, const CanFrame &frame);
    void handleGetBlock(const CanTsHeader &h, const CanFrame &frame);

    bool sendFrame(const CanTsHeader &h, const uint8_t *data, uint8_t dlc);

    void notePeerAlive(uint8_t peer);
    static size_t bitmapSizeBytes(uint8_t numBlocks);
    static bool allBlocksReceived(const std::vector<bool> &received);
    static std::vector<uint8_t> makeBitmap(const std::vector<bool> &received);
    static bool bitmapBitIsSet(const std::vector<uint8_t> &bitmap, uint8_t index);

    mutable std::mutex mtx;
    std::condition_variable cv;

    TcHandler tcHandler;
    TmHandler tmHandler;
    UTMHandler utmHandler;
    TimeSyncHandler tsHandler;
    SetBlockHandler setBlockHandler;
    GetBlockHandler getBlockHandler;

    std::unordered_map<uint8_t, BlockSession> sbSessions;
    std::unordered_map<uint8_t, BlockSession> gbSessions;
    std::unordered_map<uint8_t, PeerState> peers;

    PendingTcTm pendingTc;
    PendingTcTm pendingTm;
    PendingSetBlock pendingSb;
    PendingGetBlock pendingGb;

    std::shared_ptr<ICanFrameTransmitter> transmitter;
    uint8_t localAddress = 0;

    uint8_t lastTelemetry[CAN_FRAME_MAX_DATA] = {};
    uint8_t lastTelemetryDlc = 0;
    std::vector<uint8_t> lastGetBlockData;
};

} // namespace cannect
