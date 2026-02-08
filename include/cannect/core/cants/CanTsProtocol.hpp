#pragma once

#include "cannect/core/CanFrame.hpp"
#include "cannect/core/CanSender.hpp"
#include "cannect/core/ICanTransport.hpp"
#include "cannect/core/IProtocol.hpp"
#include <memory>
namespace cannect
{
    class CanTsProtocol : public IProtocol
    {
      public:
        CanTsProtocol(std::shared_ptr<ICanTransport> canTransport);
        ~CanTsProtocol() = default;
        void update(const CanFrame &canFrame) override;
        void send(std::vector<uint8_t> data) override;
        std::vector<uint8_t> receive() override;

      private:
        void sendTcAck(CanFrame &canFrame);
        void sendTcNack(CanFrame &canFrame);

        CanSender canSender;
    };
} // namespace cannect
