#pragma once

#include "GetBlockManager.hpp"
#include "SetBlockManager.hpp"
#include "cannect/ICanProtocol.hpp"
#include "cannect/Status.hpp"
#include "cannect/Types.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

#define DEFAULT_TIMEOUT_MS 200
#define CAN_TS_KEEPALIVE_ADDRESS 1

namespace cannect
{

class CanTsProtocol : public ICanProtocol
{
  public:
    explicit CanTsProtocol(uint8_t localAddress);

    using TcHandler = std::function<bool(uint8_t from, uint8_t channel, uint8_t request[CAN_FRAME_MAX_DATA])>;
    using TmHandler = std::function<bool(uint8_t from, uint8_t channel, uint8_t response[CAN_FRAME_MAX_DATA])>;
    using UTMHandler = std::function<void(uint8_t from, uint8_t channel, uint8_t data[CAN_FRAME_MAX_DATA])>;
    using TimeSyncHandler = std::function<void(uint8_t from, uint8_t timeLE[CAN_FRAME_MAX_DATA])>;
    using SetBlockHandler = SetBlockManager::Handler;
    using GetBlockHandler = GetBlockManager::Handler;

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

    struct PeerState
    {
        std::chrono::steady_clock::time_point lastSeen{};
    };

    void handleTelecommand(const CanTsHeader &h, const CanFrame &frame);
    void handleTelemetry(const CanTsHeader &h, const CanFrame &frame);
    void handleUnsolicited(const CanTsHeader &h, const CanFrame &frame);
    void handleTimeSync(const CanTsHeader &h, const CanFrame &frame);

    bool sendFrame(const CanTsHeader &h, const uint8_t *data, uint8_t dlc);
    void notePeerAlive(uint8_t peer);

    mutable std::mutex mtx;
    std::condition_variable cv;

    TcHandler tcHandler;
    TmHandler tmHandler;
    UTMHandler utmHandler;
    TimeSyncHandler tsHandler;

    std::unordered_map<uint8_t, PeerState> peers;

    PendingTcTm pendingTc;
    PendingTcTm pendingTm;

    std::shared_ptr<ICanFrameTransmitter> transmitter;
    uint8_t localAddress = 0;

    SetBlockManager setBlockMgr;
    GetBlockManager getBlockMgr;

    uint8_t lastTelemetry[CAN_FRAME_MAX_DATA] = {};
    uint8_t lastTelemetryDlc = 0;
};

} // namespace cannect
