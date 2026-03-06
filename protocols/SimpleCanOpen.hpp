#pragma once

#include <iostream>

namespace can {

class SimpleCanOpenProtocol : public ICanProtocol {
public:
    explicit SimpleCanOpenProtocol(uint8_t nodeId)
        : nodeId_(nodeId) {}

    const std::string& name() const noexcept override {
        static const std::string kName = "SimpleCANopen";
        return kName;
    }

    void onAttach(ProtocolContext& context) override {
        context_ = &context;
    }

    void onDetach() override {
        context_ = nullptr;
    }

    bool accepts(const CanFrame& frame) const override {
        // example: SDO / PDO / NMT ranges simplified
        return frame.id.value >= 0x000 && frame.id.value <= 0x7FF;
    }

    void handleFrame(const CanFrame& frame) override {
        if (frame.id.value == 0x700 + nodeId_) {
            std::cout << "Heartbeat received for node " << static_cast<int>(nodeId_) << "\n";
        }
    }

    void tick() override {
        // optional periodic actions
    }

    Status sendHeartbeat(const InterfaceId& iface) {
        if (!context_) {
            return {ErrorCode::NotInitialized, "Protocol not attached"};
        }

        CanFrame frame;
        frame.id = CanId{static_cast<uint32_t>(0x700 + nodeId_), FrameFormat::Standard};
        frame.size = 1;
        frame.payload[0] = 0x05;
        frame.timestamp = context_->now();

        return context_->send(iface, frame);
    }

private:
    uint8_t nodeId_;
    ProtocolContext* context_{nullptr};
};

} // namespace can
