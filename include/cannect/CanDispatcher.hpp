#pragma once

#include "cannect/ICanFrameHandler.hpp"
#include "cannect/ICanInterface.hpp"
#include "cannect/IFilter.hpp"
#include "cannect/Status.hpp"

#include <memory>
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

    Status addInterface(std::shared_ptr<ICanInterface> canInterface);
    Status addReceiver(const std::string &interfaceName, std::shared_ptr<ICanFrameHandler> receiver);
    Status addFilter(const std::string &interfaceName, std::shared_ptr<IFilter> filter);

    void start();
    void stop();

  private:
    struct DispatcherEntry
    {
        std::shared_ptr<ICanInterface>              canInterface;
        std::vector<std::shared_ptr<IFilter>>          filters;
        std::vector<std::shared_ptr<ICanFrameHandler>> receivers;
        std::unique_ptr<std::thread>                ifaceThread;
    };

    DispatcherEntry *findEntry(const std::string &interfaceName);
    void runInterface(DispatcherEntry &entry);

    std::vector<DispatcherEntry> dispatcherEntries;
    std::mutex entriesMutex;
    bool running = false;
};

} // namespace cannect
