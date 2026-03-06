#pragma once

#include <memory>
#include <string>

namespace can {

class ProtocolContext {
public:
    virtual ~ProtocolContext() = default;

    virtual Status send(const InterfaceId& interfaceId, const CanFrame& frame) = 0;
    virtual Status broadcast(const CanFrame& frame) = 0;
    virtual Timestamp now() const = 0;
};

class ICanProtocol {
public:
    virtual ~ICanProtocol() = default;

    virtual const std::string& name() const noexcept = 0;

    virtual void onAttach(ProtocolContext& context) = 0;
    virtual void onDetach() = 0;

    virtual bool accepts(const CanFrame& frame) const = 0;
    virtual void handleFrame(const CanFrame& frame) = 0;

    virtual void tick() = 0; // timers, retries, heartbeats, etc.
};

using ICanProtocolPtr = std::unique_ptr<ICanProtocol>;

} // namespace can
