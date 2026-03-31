#pragma once

#include "CanTsCodec.hpp"
#include "cannect/Types.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace cannect
{

class GetBlockManager
{
  public:
    using FrameSender = std::function<bool(const CanTsHeader &, const uint8_t *, uint8_t)>;
    using Handler = std::function<bool(uint8_t from, uint8_t channel, std::vector<uint8_t> &response)>;

    GetBlockManager(uint8_t localAddress, FrameSender sender);

    void setHandler(Handler h);
    void onFrame(const CanTsHeader &h, const CanFrame &frame);
    bool getBlock(uint8_t to, const std::vector<uint8_t> &addressLE, uint32_t timeoutMs);
    bool getLastData(std::vector<uint8_t> &out) const;

  private:
    struct Session
    {
        uint8_t peer = 0;
        uint8_t channel = 0;
        std::vector<uint8_t> address;
        uint8_t numBlocks = 0;
        std::vector<std::vector<uint8_t>> blocks;
        std::chrono::steady_clock::time_point lastActivity{};
        bool prepared = false;
        bool accepted = false;
    };

    struct PendingState
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

    uint8_t localAddress;
    FrameSender frameSender;
    Handler handler;

    mutable std::mutex mtx;
    std::condition_variable cv;

    std::unordered_map<uint8_t, Session> sessions;
    PendingState pending;
    std::vector<uint8_t> lastData;
};

} // namespace cannect
