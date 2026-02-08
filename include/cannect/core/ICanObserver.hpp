#pragma once

#include "CanFrame.hpp"

namespace cannect
{

    class ICanObserver
    {
      public:
        virtual ~ICanObserver() {};
        virtual void update(const CanFrame &canFrame) = 0;
    };

} // namespace cannect
