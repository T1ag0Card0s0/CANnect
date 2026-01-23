#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <ostream>

namespace cannect
{

  constexpr size_t Can_MAX_DATA_LENGTH = 8;

  class CanFrame
  {
  public:
    CanFrame();

    CanFrame(uint32_t canId, const uint8_t *data, uint8_t length);

    uint32_t getCanId() const;
    uint8_t getDLC() const;
    const uint8_t *getData() const;
    uint8_t *getData();

    void setCanId(uint32_t canId);
    void setDLC(uint8_t dlc);

    void setData(const uint8_t *data, uint8_t length);

    friend std::ostream &operator<<(std::ostream &out, const CanFrame &c);

  private:
    uint32_t m_canId;
    uint8_t m_dlc;
    std::array<uint8_t, Can_MAX_DATA_LENGTH> m_data;
  };

  std::ostream &operator<<(std::ostream &out, const CanFrame &frame);

} // namespace cannect
