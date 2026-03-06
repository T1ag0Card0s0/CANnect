#pragma once

#include "Types.hpp"

class IRxSink {
public:
    virtual ~IRxSink() = default;
    virtual void onFrameReceived(const CanFrame& frame) = 0;
};
