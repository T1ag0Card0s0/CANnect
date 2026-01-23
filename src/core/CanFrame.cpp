#include "cannect/core/CanFrame.hpp"

#include <iomanip>

using namespace cannect;

CanFrame::CanFrame() : m_canId(0), m_dlc(0), m_data{}
{
}

CanFrame::CanFrame(uint32_t canId, const uint8_t *data, uint8_t length)
    : m_canId(canId), m_dlc(length > Can_MAX_DATA_LENGTH ? Can_MAX_DATA_LENGTH : length), m_data{}
{
  if (data != nullptr && m_dlc > 0)
  {
    std::memcpy(m_data.data(), data, m_dlc);
  }
}

uint32_t CanFrame::getCanId() const
{
  return m_canId;
}

uint8_t CanFrame::getDLC() const
{
  return m_dlc;
}

const uint8_t *CanFrame::getData() const
{
  return m_data.data();
}

uint8_t *CanFrame::getData()
{
  return m_data.data();
}

void CanFrame::setCanId(uint32_t canId)
{
  m_canId = canId;
}

void CanFrame::setDLC(uint8_t dlc)
{
  m_dlc = dlc > Can_MAX_DATA_LENGTH ? Can_MAX_DATA_LENGTH : dlc;
}

void CanFrame::setData(const uint8_t *data, uint8_t length)
{
  m_dlc = length > Can_MAX_DATA_LENGTH ? Can_MAX_DATA_LENGTH : length;
  if (data != nullptr && m_dlc > 0)
  {
    std::memcpy(m_data.data(), data, m_dlc);
  }
}

namespace cannect
{
  std::ostream &operator<<(std::ostream &out, const CanFrame &frame)
  {
    out << "0x" << std::hex << std::setw(3) << std::setfill('0') << frame.getCanId() << " [" << std::dec
        << static_cast<int>(frame.getDLC()) << "] ";
    for (uint8_t i = 0; i < frame.getDLC(); ++i)
    {
      out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(frame.getData()[i]) << " ";
    }
    out << std::dec << std::endl;
    return out;
  }
} // namespace cannect
