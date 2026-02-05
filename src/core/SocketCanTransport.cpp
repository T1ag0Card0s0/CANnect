#include "cannect/core/SocketCanTransport.hpp"
#include <cstring>
#include <iostream>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace cannect;

SocketCanTransport::~SocketCanTransport()
{
  if (m_isOpen)
  {
    close();
  }
}

bool SocketCanTransport::open()
{
  if (m_isOpen)
  {
    std::cerr << "Socket already open" << std::endl;
    return false;
  }

  if (m_interface.empty())
  {
    std::cerr << "Interface not set" << std::endl;
    return false;
  }

  fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (fd < 0)
  {
    std::cerr << "Failed to create socket" << std::endl;
    return false;
  }

  struct ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, m_interface.c_str(), IFNAMSIZ - 1);

  if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0)
  {
    std::cerr << "Failed to get interface index for " << m_interface << std::endl;
    ::close(fd);
    fd = -1;
    return false;
  }

  struct sockaddr_can addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  if (bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
  {
    std::cerr << "Failed to bind socket" << std::endl;
    ::close(fd);
    fd = -1;
    return false;
  }

  m_isOpen = true;
  return true;
}

bool SocketCanTransport::close()
{
  if (!m_isOpen)
  {
    return false;
  }

  if (::close(fd) < 0)
  {
    std::cerr << "Failed to close socket" << std::endl;
    return false;
  }

  fd = -1;
  m_isOpen = false;
  return true;
}

int SocketCanTransport::writeFrame(const CanFrame &frame)
{
  if (!m_isOpen)
  {
    std::cerr << "Socket not open" << std::endl;
    return -1;
  }

  struct can_frame kernelFrame;
  std::memset(&kernelFrame, 0, sizeof(kernelFrame));

  uint32_t canId = frame.getCanId();
  if (canId > CAN_SFF_MASK)
  {
    kernelFrame.can_id = canId | CAN_EFF_FLAG;
  }
  else
  {
    kernelFrame.can_id = canId;
  }
  kernelFrame.can_dlc = frame.getDLC();
  std::memcpy(kernelFrame.data, frame.getData(), frame.getDLC());

  int bytesWritten = ::write(fd, &kernelFrame, sizeof(struct can_frame));

  if (bytesWritten != sizeof(struct can_frame))
  {
    std::cerr << "Failed to write Can frame" << std::endl;
    return -1;
  }

  return bytesWritten;
}

int SocketCanTransport::readFrame(CanFrame &frame)
{
  if (!m_isOpen)
  {
    std::cerr << "Socket not open" << std::endl;
    return -1;
  }

  struct can_frame kernelFrame;
  std::memset(&kernelFrame, 0, sizeof(kernelFrame));

  int nbytes = ::read(fd, &kernelFrame, sizeof(struct can_frame));

  if (nbytes < 0)
  {
    std::cerr << "Failed to read Can frame" << std::endl;
    return -1;
  }

  if (nbytes < static_cast<int>(sizeof(struct can_frame)))
  {
    std::cerr << "Incomplete Can frame received" << std::endl;
    return -1;
  }

  // Mask out special flags (EFF, RTR, ERR) to get the actual CAN ID
  uint32_t canId = kernelFrame.can_id & CAN_EFF_MASK;
  frame.setCanId(canId);
  frame.setDLC(kernelFrame.can_dlc);
  frame.setData(kernelFrame.data, kernelFrame.can_dlc);

  return nbytes;
}

bool SocketCanTransport::setFilters(const std::vector<CanFilter> &filters)
{
  if (!m_isOpen)
  {
    std::cerr << "Socket not open" << std::endl;
    return false;
  }

  if (filters.empty())
  {
    return true;
  }

  std::vector<struct can_filter> kernelFilters;
  kernelFilters.reserve(filters.size());

  for (const auto &filter : filters)
  {
    struct can_filter kf;
    kf.can_id = filter.id;
    kf.can_mask = filter.mask;
    kernelFilters.push_back(kf);
  }

  if (setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, kernelFilters.data(),
                 kernelFilters.size() * sizeof(struct can_filter)) < 0)
  {
    std::cerr << "Failed to set Can filters" << std::endl;
    return false;
  }

  return true;
}

void SocketCanTransport::setInterface(const std::string &interface)
{
  if (m_isOpen)
  {
    std::cerr << "Cannot change interface while socket is open" << std::endl;
    return;
  }
  m_interface = interface;
}
