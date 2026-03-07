#pragma once

#include "cannect/ICanFrameHandler.hpp"
#include "cannect/ICanInterface.hpp"
#include "cannect/IFilter.hpp"
#include "cannect/Status.hpp"

#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace cannect
{

class CanDispatcher
{
  public:
    CanDispatcher() = default;
    ~CanDispatcher();

    Status addInterface(ICanInterface &canInterface);
    Status addReceiver(const std::string &interfaceName, ICanFrameHandler &receiver);
    Status addFilter(const std::string &interfaceName, IFilter &filter);

    void start();
    void stop();

  private:
    struct DispatcherEntry
    {
        ICanInterface &canInterface;
        std::vector<std::reference_wrapper<IFilter>> filters;
        std::vector<std::reference_wrapper<ICanFrameHandler>> receivers;
        std::thread ifaceThread;
    };

    DispatcherEntry *findEntry(const std::string &interfaceName);

    void runInterface(DispatcherEntry &entry);

    std::vector<DispatcherEntry> dispatcherEntries;
    std::mutex entriesMutex;
    bool running = false;
};

} // namespace cannect
