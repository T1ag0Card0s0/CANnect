#pragma once

#include <functional>
#include <mutex>

#include "cannect/core/CanFrame.hpp"
#include "cannect/core/CanSender.hpp"
#include "cannect/core/ICanTransport.hpp"
#include "cannect/core/IProtocol.hpp"
#include "cannect/core/cants/Types.hpp"

namespace cannect
{
    namespace cants
    {

class CanTsProtocol : public IProtocol
{
  public:

    using TcHandler = std::function<bool(uint8_t from, uint8_t channel, const std::vector<uint8_t> &args, std::vector<uint8_t> &response)>;
    using TmHandler = std::function<bool(uint8_t from, uint8_t channel, std::vector<uint8_t> &value)>;
    using UTMHandler = std::function<void(uint8_t from, uint8_t channel, const std::vector<uint8_t> &value)>;
    using TimeSyncHandler = std::function<void(uint8_t from, const std::vector<uint8_t> &timeLE)>;

    explicit CanTsProtocol(ICanTransport &transport, uint8_t localAddress);
    ~CanTsProtocol() = default;

    void update(const CanFrame &frame) override;
    void send(const std::vector<uint8_t> &data) override;
    std::vector<uint8_t> receive() override;

    void setTelecommandHandler(TcHandler h);
    void setTelemetryHandler(TmHandler h);
    void setUnsolicitedTelemetryHandler(UTMHandler h);
    void setTimeSyncHandler(TimeSyncHandler h);

    bool sendTelecommand(uint8_t to, uint8_t channel, const std::vector<uint8_t> &args);
    bool requestTelemetry(uint8_t to, uint8_t channel);
    bool sendUnsolicitedTelemetry(uint8_t to, uint8_t channel, const std::vector<uint8_t> &value);
    bool broadcastTimeSync(const std::vector<uint8_t> &timeLE);

    bool setBlock(uint8_t to, const std::vector<uint8_t> &addressLE, const std::vector<uint8_t> &data, uint32_t timeoutMs = 200);
    bool getBlock(uint8_t to, const std::vector<uint8_t> &addressLE, uint8_t numBlocks, std::vector<uint8_t> &out, uint32_t timeoutMs = 200);

  private:
    bool sendFrame(uint8_t to, CanTsMessageType type, uint16_t command, const uint8_t *data, uint8_t len);

    void handleTelecommand(const CanTsHeader &h, const CanFrame &frame);
    void handleTelemetry(const CanTsHeader &h, const CanFrame &frame);
    void handleUnsolicited(const CanTsHeader &h, const CanFrame &frame);
    void handleTimeSync(const CanTsHeader &h, const CanFrame &frame);
    void handleSetBlock(const CanTsHeader &h, const CanFrame &frame);
    void handleGetBlock(const CanTsHeader &h, const CanFrame &frame);

    struct SetBlockSession
    {
        std::vector<uint8_t> address;
        uint8_t numBlocks = 0;
        std::vector<std::vector<uint8_t>> blocks;
        std::vector<uint8_t> bitmap;
        bool done = false;
    };

    struct GetBlockSession
    {
        std::vector<uint8_t> address;
        uint8_t numBlocks = 0;
        std::vector<std::vector<uint8_t>> blocks;
    };

    TcHandler tcHandler;
    TmHandler tmHandler;
    UTMHandler utmHandler;
    TimeSyncHandler tsHandler;

    std::unordered_map<uint8_t, SetBlockSession> sbSessions;
    std::unordered_map<uint8_t, GetBlockSession> gbSessions;

    CanSender sender;

    uint8_t local;
    std::mutex mtx;
};
}
} // namespace cannect
