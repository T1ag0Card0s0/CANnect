#pragma once

#include "cannect/Types.hpp"

namespace cannect
{

class IFilter
{
  public:
    virtual ~IFilter() = default;

    virtual bool isValid(const CanFrame &frame) = 0;
};

} // namespace cannect
