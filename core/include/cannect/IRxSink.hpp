#pragma once

namespace can {

class IRxSink {
public:
    virtual ~IRxSink() = default;
    virtual void onFrameReceived(const CanFrame& frame) = 0;
};

} // namespace can
