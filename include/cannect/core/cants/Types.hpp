#pragma once

#include <cstdint>

#define CANTS_FRAME_MAX_DATA_SIZE 8

namespace cannect
{
  namespace cants
  {

    enum class CanTsMessageType : uint8_t
    {
      TELECOMMAND = 0,
      TELEMETRY = 1,
      UNSOLICITED = 2,
      TIMESYNC = 3,
      SETBLOCK = 4,
      GETBLOCK = 5,
    };

    struct __attribute__((packed)) CanTsHeader
    {
      uint8_t to;
      uint8_t from;
      CanTsMessageType type;
      uint8_t command;
    };

    struct __attribute__((packed)) CanTsFrame
    {
      CanTsHeader h;
      uint8_t data[CANTS_FRAME_MAX_DATA_SIZE];
      uint8_t len;
    };

  } // namespace cants
} // namespace cannect
