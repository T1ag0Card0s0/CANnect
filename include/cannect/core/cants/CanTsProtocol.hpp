#pragma once

#include "cannect/core/CanFrame.hpp"
#include "cannect/core/CanSender.hpp"
#include "cannect/core/ICanTransport.hpp"
#include "cannect/core/IProtocol.hpp"

namespace cannect
{
    class CanTsProtocol : public IProtocol
    {
      public:
        explicit CanTsProtocol(ICanTransport &transport);
        ~CanTsProtocol() = default;

        void                 update(const CanFrame &frame) override;
        void                 send(std::vector<uint8_t> data) override;
        std::vector<uint8_t> receive() override;

      private:
        void sendTcAck(const CanFrame &frame);
        void sendTcNack(const CanFrame &frame);

        CanSender sender;
    };
} // namespace cannect
