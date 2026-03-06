#pragma once

#include <memory>
#include <string>

namespace can {

struct InterfaceConfig {
    uint32_t nominalBitrate{500000};
    uint32_t dataBitrate{2000000};
    bool fdEnabled{false};
    std::string device{};
};

class ICanInterface {
public:
    virtual ~ICanInterface() = default;

    virtual const InterfaceId& id() const noexcept = 0;

    virtual Status initialize(const InterfaceConfig& config) = 0;
    virtual Status start() = 0;
    virtual Status stop() = 0;

    virtual Status send(const CanFrame& frame) = 0;

    virtual void setReceiveSink(IRxSink* sink) = 0;
};

using ICanInterfacePtr = std::unique_ptr<ICanInterface>;

class ICanInterfaceFactory {
public:
    virtual ~ICanInterfaceFactory() = default;
    virtual ICanInterfacePtr create(const InterfaceId& id) = 0;
};

} // namespace can
