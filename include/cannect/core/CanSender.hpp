#pragma once

#include "CanFrame.hpp"
#include "ICanTransport.hpp"

#include <memory>

namespace cannect
{
    class CanSender
    {
      public:
        explicit CanSender(std::shared_ptr<ICanTransport> transport) : transport(transport)
        {
        }

        ~CanSender() = default;

        int sendFrame(const CanFrame &frame);

      private:
        std::shared_ptr<ICanTransport> transport;
    };
} // namespace cannect
