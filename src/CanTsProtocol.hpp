#pragma once

#include "cannect/ICanProtocol.hpp"
#include "cannect/Status.hpp"
#include "cannect/Types.hpp"

#include <vector>
#include <unordered_map>
#include <functional>

#define DEFAULT_TIMEOUT_MS 200

namespace cannect
{

class CanTsProtocol : public ICanProtocol
{
  public:
    CanTsProtocol() = default;

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
    bool setBlock(uint8_t to, const std::vector<uint8_t> &addressLE, const std::vector<uint8_t> &data, uint32_t timeoutMs = DEFAULT_TIMEOUT_MS);
    bool getBlock(uint8_t to, const std::vector<uint8_t> &addressLE, uint32_t timeoutMs = DEFAULT_TIMEOUT_MS);

  private:
    struct SetBlockSession
    {

    };
  
    struct GetBlockSession
    {

    };

    TcHandler tcHandler;
    TmHandler tmHandler;
    UTMHandler utmHandler;
    TimeSyncHandler tsHandler;
    std::unordered_map<uint8_t, SetBlockSession> sbSessions;
    std::unordered_map<uint8_t, GetBlockSession> gbSessions;
    
    std::shared_ptr<ICanFrameTransmitter> transmitter;

};

} // namespace cannect
