/**
 * @file CanDispatcher.hpp
 * @brief Declares the component that receives frames from interfaces and forwards them to handlers.
 */

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

/**
 * @brief Manages one or more CAN interfaces and dispatches accepted frames to receivers.
 */
class CanDispatcher
{
  public:
    /**
     * @brief Creates an idle dispatcher.
     */
    CanDispatcher() = default;

    /**
     * @brief Stops the dispatcher and joins all worker threads.
     */
    ~CanDispatcher();

    /**
     * @brief Registers a CAN interface with the dispatcher.
     *
     * Interfaces must be added before start() is called.
     *
     * @param canInterface Interface instance to manage.
     * @return Status::SUCCESS when the interface is added, otherwise an error code.
     */
    Status addInterface(std::shared_ptr<ICanInterface> canInterface);

    /**
     * @brief Registers a frame receiver for a named interface.
     *
     * @param interfaceName Name returned by ICanInterface::getName().
     * @param receiver Receiver invoked for accepted frames.
     * @return Status::SUCCESS when the receiver is registered, otherwise an error code.
     */
    Status addReceiver(const std::string &interfaceName, std::shared_ptr<ICanFrameHandler> receiver);

    /**
     * @brief Registers a filter for a named interface.
     *
     * All filters attached to an interface must accept a frame before it is dispatched.
     *
     * @param interfaceName Name returned by ICanInterface::getName().
     * @param filter Filter evaluated before receivers are notified.
     * @return Status::SUCCESS when the filter is registered, otherwise an error code.
     */
    Status addFilter(const std::string &interfaceName, std::shared_ptr<IFilter> filter);

    /**
     * @brief Opens each registered interface and starts one worker thread per interface.
     */
    void start();

    /**
     * @brief Requests all worker threads to stop, closes the interfaces, and joins the threads.
     */
    void stop();

  private:
    struct DispatcherEntry
    {
        std::shared_ptr<ICanInterface> canInterface;
        std::vector<std::shared_ptr<IFilter>> filters;
        std::vector<std::shared_ptr<ICanFrameHandler>> receivers;
        std::unique_ptr<std::thread> ifaceThread;
    };

    DispatcherEntry *findEntry(const std::string &interfaceName);
    void runInterface(DispatcherEntry &entry);

    std::vector<DispatcherEntry> dispatcherEntries;
    std::mutex entriesMutex;
    bool running = false;
};

} // namespace cannect
