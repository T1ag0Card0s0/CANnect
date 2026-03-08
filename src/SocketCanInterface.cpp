#include "SocketCanInterface.hpp"

#include "cannect/Logger.hpp"
#include "cannect/Status.hpp"

#include <cerrno>
#include <cstring>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

using namespace cannect;

SocketCanInterface::SocketCanInterface(std::string name) : name(name), isOpen(false), fd(-1)
{
    LOG_DEBUG("SocketCanInterface created for interface " + name);
}

SocketCanInterface::~SocketCanInterface()
{
    if (!this->isClosed())
    {
        LOG_DEBUG("Destructor closing interface " + this->name + " that was still open");
        this->close();
    }
}

Status SocketCanInterface::open()
{
    if (this->isOpen)
    {
        LOG_WARNING("Attempted to open " + this->name + " but it is already open");
        return Status::INTERFACE_ALREADY_OPEN;
    }
    if (this->name.empty())
    {
        LOG_ERROR("Attempted to open interface with empty name");
        return Status::INTERFACE_INVALID_NAME;
    }

    this->fd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0)
    {
        LOG_ERROR("socket() failed for " + this->name + ": " + std::strerror(errno));
        return Status::UNSUCCESS;
    }
    LOG_DEBUG("Raw CAN socket created (fd=" + std::to_string(this->fd) + ")");

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, this->name.c_str(), IFNAMSIZ - 1);

    if (::ioctl(this->fd, SIOCGIFINDEX, &ifr) < 0)
    {
        LOG_ERROR("ioctl(SIOCGIFINDEX) failed for " + this->name + ": " + std::strerror(errno));
        ::close(this->fd);
        this->fd = -1;
        return Status::UNSUCCESS;
    }
    LOG_DEBUG("Interface " + this->name + " resolved to ifindex=" + std::to_string(ifr.ifr_ifindex));

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (::bind(this->fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        LOG_ERROR("bind() failed for " + this->name + ": " + std::strerror(errno));
        ::close(this->fd);
        this->fd = -1;
        return Status::UNSUCCESS;
    }

    if (::pipe(pipeFds) < 0)
    {
        LOG_ERROR("pipe() failed for " + this->name + ": " + std::strerror(errno));
        ::close(this->fd);
        this->fd = -1;
        return Status::UNSUCCESS;
    }

    this->isOpen = true;
    LOG_INFO("Interface " + this->name + " opened successfully (fd=" + std::to_string(this->fd) + ")");
    return Status::SUCCESS;
}

Status SocketCanInterface::close()
{
    if (!this->isOpen)
    {
        LOG_WARNING("close() called on " + this->name + " which is already closed");
        return Status::UNSUCCESS;
    }

    if (pipeFds[1] >= 0)
    {
        ::write(pipeFds[1], "x", 1);
        ::close(pipeFds[0]);
        ::close(pipeFds[1]);
        pipeFds[0] = pipeFds[1] = -1;
    }

    if (this->fd >= 0 && ::close(this->fd) < 0)
    {
        LOG_ERROR("close(fd=" + std::to_string(this->fd) + ") failed for " + this->name + "': " + std::strerror(errno));
        return Status::UNSUCCESS;
    }

    LOG_INFO("Interface " + this->name + " closed (fd=" + std::to_string(this->fd) + ")");
    this->fd = -1;
    this->isOpen = false;
    return Status::SUCCESS;
}

Status SocketCanInterface::receive(CanFrame &canFrame)
{
    if (!this->isOpen)
    {
        LOG_WARNING("receive() called on closed interface " + this->name);
        return Status::UNSUCCESS;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(this->fd, &readfds);
    FD_SET(pipeFds[0], &readfds);

    int ret = ::select(std::max(this->fd, pipeFds[0]) + 1, &readfds, nullptr, nullptr, nullptr);
    if (ret < 0)
    {
        if (errno == EINTR)
        {
            return Status::SUCCESS;
        }
        LOG_ERROR("select() failed on " + this->name + ": " + std::strerror(errno));
        return Status::UNSUCCESS;
    }

    if (FD_ISSET(pipeFds[0], &readfds) || !this->isOpen)
    {
        LOG_DEBUG("receive() interrupted by shutdown on " + this->name);
        return Status::UNSUCCESS;
    }

    struct can_frame kernelFrame;
    std::memset(&kernelFrame, 0, sizeof(kernelFrame));

    int nbytes = ::read(this->fd, &kernelFrame, sizeof(struct can_frame));
    if (nbytes < 0)
    {
        LOG_ERROR("read() failed on " + this->name + ": " + std::strerror(errno));
        return Status::UNSUCCESS;
    }

    if (nbytes < static_cast<int>(sizeof(struct can_frame)))
    {
        LOG_ERROR("read() returned incomplete frame (" + std::to_string(nbytes) + " bytes) on " + this->name);
        return Status::UNSUCCESS;
    }

    canFrame.id = kernelFrame.can_id & CAN_EFF_MASK;
    canFrame.dlc = kernelFrame.can_dlc;
    std::memcpy(canFrame.data, kernelFrame.data, kernelFrame.can_dlc);

    LOG_DEBUG("Frame received on " + this->name + ": id=0x" +
              [&] {
                  char buf[9];
                  snprintf(buf, sizeof(buf), "%08X", canFrame.id);
                  return std::string(buf);
              }() +
              " dlc=" + std::to_string(canFrame.dlc));

    return Status::SUCCESS;
}

Status SocketCanInterface::send(const CanFrame &canFrame)
{
    if (!this->isOpen)
    {
        LOG_WARNING("send() called on closed interface " + this->name);
        return Status::UNSUCCESS;
    }

    struct can_frame kernelFrame;
    std::memset(&kernelFrame, 0, sizeof(kernelFrame));

    if (canFrame.id <= CAN_SFF_MASK)
    {
        kernelFrame.can_id = canFrame.id;
    }
    else if (canFrame.id <= CAN_EFF_MASK)
    {
        kernelFrame.can_id = canFrame.id | CAN_EFF_FLAG;
    }
    else
    {
        LOG_ERROR("send() rejected frame: id=0x" + std::to_string(canFrame.id) + " exceeds CAN_EFF_MASK");
        return Status::UNSUCCESS;
    }

    if (canFrame.dlc > 8)
    {
        LOG_ERROR("send() rejected frame: dlc=" + std::to_string(canFrame.dlc) + " exceeds maximum of 8");
        return Status::UNSUCCESS;
    }

    kernelFrame.can_dlc = canFrame.dlc;
    std::memcpy(kernelFrame.data, canFrame.data, canFrame.dlc);

    int bytesWritten = ::write(this->fd, &kernelFrame, sizeof(struct can_frame));
    if (bytesWritten != static_cast<int>(sizeof(struct can_frame)))
    {
        LOG_ERROR("write() failed on " + this->name + ": wrote " + std::to_string(bytesWritten) + "/" +
                  std::to_string(sizeof(struct can_frame)) + " bytes: " + std::strerror(errno));
        return Status::UNSUCCESS;
    }

    LOG_DEBUG("Frame sent on " + this->name + ": id=0x" +
              [&] {
                  char buf[9];
                  snprintf(buf, sizeof(buf), "%08X", canFrame.id);
                  return std::string(buf);
              }() +
              " dlc=" + std::to_string(canFrame.dlc));
    return Status::SUCCESS;
}

bool SocketCanInterface::isClosed() const { return !this->isOpen; }
std::string SocketCanInterface::getName() { return this->name; }
