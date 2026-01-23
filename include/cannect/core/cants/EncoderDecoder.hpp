#pragma once

#include "cannect/cants/Types.hpp"
#include <cstdint>

namespace cannect
{
  namespace cants
  {

    class EncoderDecoder
    {
    public:
      static constexpr uint32_t TO_SHIFT = 21;
      static constexpr uint32_t FROM_SHIFT = 13;
      static constexpr uint32_t TYPE_SHIFT = 10;
      static constexpr uint32_t CMD_SHIFT = 2;

      static constexpr uint32_t TO_MASK = 0xFFu << TO_SHIFT;
      static constexpr uint32_t FROM_MASK = 0xFFu << FROM_SHIFT;
      static constexpr uint32_t TYPE_MASK = 0x7u << TYPE_SHIFT;
      static constexpr uint32_t CMD_MASK = 0xFFu << CMD_SHIFT;

      static uint32_t encode(const CanTsHeader &h)
      {
        uint32_t id = 0;

        id |= (uint32_t(h.to) << TO_SHIFT) & TO_MASK;
        id |= (uint32_t(h.from) << FROM_SHIFT) & FROM_MASK;
        id |= (uint32_t(h.type) << TYPE_SHIFT) & TYPE_MASK;
        id |= (uint32_t(h.command) << CMD_SHIFT) & CMD_MASK;

        return id;
      }

      static CanTsHeader decode(uint32_t can_id)
      {
        CanTsHeader h{};

        h.to = uint8_t((can_id & TO_MASK) >> TO_SHIFT);
        h.from = uint8_t((can_id & FROM_MASK) >> FROM_SHIFT);
        h.type = static_cast<CanTsMessageType>((can_id & TYPE_MASK) >> TYPE_SHIFT);
        h.command = uint8_t((can_id & CMD_MASK) >> CMD_SHIFT);

        return h;
      }
    };

  } // namespace cants
} // namespace cannect
