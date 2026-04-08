/**
 * @file SocketCanInterface.hpp
 * @brief Declares the Linux SocketCAN transport implementation.
 */

#pragma once

#include "cannect/ICanInterface.hpp"

#include <string>

namespace cannect
{

/**
 * @brief Implements ICanInterface using Linux raw CAN sockets.
 */
class SocketCanInterface : public ICanInterface
{
  public:
    /**
     * @brief Creates a SocketCAN transport bound to a named network interface.
     *
     * @param name Interface name such as `can0`.
     */
    explicit SocketCanInterface(std::string name);

    /**
     * @brief Closes the interface if it is still open.
     */
    ~SocketCanInterface();

    /**
     * @brief Opens the raw CAN socket and binds it to the configured interface.
     *
     * @return Status::SUCCESS when the socket is ready, otherwise an error code.
     */
    Status open() override;

    /**
     * @brief Closes the socket and interrupts any blocking receive operation.
     *
     * @return Status::SUCCESS when the interface is closed, otherwise an error code.
     */
    Status close() override;

    /**
     * @brief Waits for and reads one CAN frame from the socket.
     *
     * @param canFrame Output frame populated on success.
     * @return Status::SUCCESS when a frame is received, otherwise an error code.
     */
    Status receive(CanFrame &canFrame) override;

    /**
     * @brief Writes one CAN frame to the socket.
     *
     * @param canFrame Frame to transmit.
     * @return Status::SUCCESS when the frame is written, otherwise an error code.
     */
    Status send(const CanFrame &canFrame) override;

    /**
     * @brief Returns the configured network interface name.
     *
     * @return Interface name.
     */
    std::string getName() override;

    /**
     * @brief Reports whether the socket is currently closed.
     *
     * @return true when the interface is not open, otherwise false.
     */
    bool isClosed() const override;

  private:
    std::string name;
    bool isOpen;
    int fd;
    int pipeFds[2] = {-1, -1};
};

} // namespace cannect
