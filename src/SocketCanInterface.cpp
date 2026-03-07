#include "SocketCanInterface.hpp"

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

SocketCanInterface::SocketCanInterface(std::string name) : name(name), isOpen(false), fd(-1) {}

SocketCanInterface::~SocketCanInterface()
{
    if (!this->isClosed())
    {
        this->close();
    }
}

Status SocketCanInterface::open()
{
    if (this->isOpen)
    {
        return Status::INTERFACE_ALREADY_OPEN;
    }

    if (this->name.empty())
    {
        return Status::INTERFACE_INVALID_NAME;
    }

    this->fd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0)
    {
        return Status::UNSUCCESS;
    }

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, this->name.c_str(), IFNAMSIZ - 1);

    if (::ioctl(this->fd, SIOCGIFINDEX, &ifr) < 0)
    {
        ::close(this->fd);
        this->fd = -1;
        return Status::UNSUCCESS;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (::bind(this->fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        ::close(this->fd);
        this->fd = -1;
        return Status::UNSUCCESS;
    }

    this->isOpen = true;

    return Status::SUCCESS;
}

Status SocketCanInterface::close()
{
    if (!this->isOpen)
    {
        return Status::UNSUCCESS;
    }

    if (this->fd >= 0 && ::close(this->fd) < 0)
    {
        return Status::UNSUCCESS;
    }

    this->fd = -1;
    this->isOpen = false;
    return Status::SUCCESS;
}

Status SocketCanInterface::receive(CanFrame &canFrame)
{
    if (!this->isOpen)
    {
        return Status::UNSUCCESS;
    }

    struct can_frame kernelFrame;
    std::memset(&kernelFrame, 0, sizeof(kernelFrame));

    int nbytes = ::read(fd, &kernelFrame, sizeof(struct can_frame));

    if (nbytes < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return Status::SUCCESS;
        }
        return Status::UNSUCCESS;
    }

    if (nbytes < static_cast<int>(sizeof(struct can_frame)))
    {
        return Status::UNSUCCESS;
    }
    canFrame.id = kernelFrame.can_id & CAN_EFF_MASK;
    canFrame.dlc = kernelFrame.can_dlc;
    memcpy(canFrame.data.data(), kernelFrame.data, kernelFrame.can_dlc);
    return Status::SUCCESS;
}

Status SocketCanInterface::send(const CanFrame &canFrame)
{
    if (!this->isOpen)
    {
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
        return Status::UNSUCCESS;
    }

    if (canFrame.dlc > 8)
    {
        return Status::UNSUCCESS;
    }

    kernelFrame.can_dlc = canFrame.dlc;
    std::memcpy(kernelFrame.data, canFrame.data.data(), canFrame.dlc);

    int bytesWritten = ::write(this->fd, &kernelFrame, sizeof(struct can_frame));

    if (bytesWritten != sizeof(struct can_frame))
    {
        return Status::UNSUCCESS;
    }

    return Status::SUCCESS;
}

bool SocketCanInterface::isClosed() const { return !this->isOpen; }

std::string SocketCanInterface::getName() { return this->name; }
