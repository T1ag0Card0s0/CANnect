#pragma once

#include <cstdint>
#include <vector>

#include "cannect/core/ICanObserver.hpp"

namespace cannect
{

    class IProtocol : public ICanObserver
    {
      public:
        virtual ~IProtocol()                                         = default;
        virtual void                 send(std::vector<uint8_t> data) = 0;
        virtual std::vector<uint8_t> receive()                       = 0;
    };

} // namespace cannect
