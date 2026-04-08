/**
 * @file Cannect.hpp
 * @brief Declares the high-level facade used to configure and run CANnect.
 */

#pragma once

#include "cannect/CanDispatcher.hpp"
#include "cannect/ICanFrameHandler.hpp"
#include "cannect/ICanInterface.hpp"
#include "cannect/Status.hpp"

#include <memory>
#include <string>

namespace cannect
{

/**
 * @brief Coordinates dispatcher startup, shutdown, and signal-based waiting.
 */
class Cannect
{
  public:
    /**
     * @brief Creates an idle CANnect application facade.
     */
    Cannect() = default;

    /**
     * @brief Stops the dispatcher if the facade is still running.
     */
    ~Cannect();

    /**
     * @brief Adds a transport interface to the managed dispatcher.
     *
     * @param canInterface Interface instance to register.
     * @return Status::SUCCESS when the interface is added, otherwise an error code.
     */
    Status addInterface(std::shared_ptr<ICanInterface> canInterface);

    /**
     * @brief Adds a frame handler to a named interface.
     *
     * @param name Name of the interface that should feed the handler.
     * @param frameHandler Handler invoked for accepted frames.
     * @return Status::SUCCESS when the handler is added, otherwise an error code.
     */
    Status addHandler(std::string name, std::shared_ptr<ICanFrameHandler> frameHandler);

    /**
     * @brief Adds a frame filter to a named interface.
     *
     * @param interfaceName Name of the interface that should use the filter.
     * @param filter Filter evaluated before frames reach handlers.
     * @return Status::SUCCESS when the filter is added, otherwise an error code.
     */
    Status addFilter(const std::string &interfaceName, std::shared_ptr<IFilter> filter);

    /**
     * @brief Blocks until SIGINT or SIGTERM is received, then stops the dispatcher.
     *
     * @return Status::SUCCESS after shutdown completes.
     */
    Status waitSignal();

    /**
     * @brief Starts the managed dispatcher.
     *
     * @return Status::SUCCESS when startup begins, otherwise an error code.
     */
    Status start();

    /**
     * @brief Stops the managed dispatcher.
     */
    void stop();

  private:
    CanDispatcher dispatcher;
    bool running = false;
};

} // namespace cannect
