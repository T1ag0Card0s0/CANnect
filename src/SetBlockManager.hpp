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

class SetBlockManager
{
  public:
    using FrameSender = std::function<bool(const CanTsHeader &, const uint8_t *, uint8_t)>;
    using Handler = std::function<bool(uint8_t from, uint8_t channel, std::vector<uint8_t> &request)>;

    SetBlockManager(uint8_t localAddress, FrameSender sender);

    void setHandler(Handler h);
    void onFrame(const CanTsHeader &h, const CanFrame &frame);
    bool setBlock(uint8_t to, const std::vector<uint8_t> &addressLE, const std::vector<uint8_t> &data,
                  uint32_t timeoutMs);

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
        bool done = false;
        bool accepted = false;
    };

    struct PendingState
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

    uint8_t localAddress;
    FrameSender frameSender;
    Handler handler;

    std::mutex mtx;
    std::condition_variable cv;

    std::unordered_map<uint8_t, Session> sessions;
    PendingState pending;
};

} // namespace cannect
