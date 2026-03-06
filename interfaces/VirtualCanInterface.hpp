#pragma once

#include <iostream>

namespace can {

class VirtualCanInterface : public ICanInterface {
public:
    explicit VirtualCanInterface(InterfaceId id)
        : id_(std::move(id)) {}

    const InterfaceId& id() const noexcept override {
        return id_;
    }

    Status initialize(const InterfaceConfig&) override {
        initialized_ = true;
        return Status::Ok();
    }

    Status start() override {
        if (!initialized_) {
            return {ErrorCode::NotInitialized, "Interface not initialized"};
        }
        running_ = true;
        return Status::Ok();
    }

    Status stop() override {
        running_ = false;
        return Status::Ok();
    }

    Status send(const CanFrame& frame) override {
        if (!running_) {
            return {ErrorCode::NotInitialized, "Interface not running"};
        }

        std::cout << "[VirtualCanInterface:" << id_ << "] TX ID=0x"
                  << std::hex << frame.id.value << std::dec
                  << " LEN=" << static_cast<int>(frame.size) << "\n";
        return Status::Ok();
    }

    void setReceiveSink(IRxSink* sink) override {
        sink_ = sink;
    }

    void inject(const CanFrame& frame) {
        if (sink_) {
            auto copy = frame;
            copy.sourceInterface = id_;
            sink_->onFrameReceived(copy);
        }
    }

private:
    InterfaceId id_;
    bool initialized_{false};
    bool running_{false};
    IRxSink* sink_{nullptr};
};

} // namespace can
