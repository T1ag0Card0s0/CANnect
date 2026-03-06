#pragma once

#include "Types.hpp"

#include <cstdint>

class IFrameFilter {
public:
    virtual ~IFrameFilter() = default;
    virtual bool matches(const CanFrame& frame) const = 0;
};

class AcceptAllFilter : public IFrameFilter {
public:
    bool matches(const CanFrame&) const override { return true; }
};

class IdRangeFilter : public IFrameFilter {
public:
    IdRangeFilter(uint32_t minId, uint32_t maxId)
        : minId_(minId), maxId_(maxId) {}

    bool matches(const CanFrame& frame) const override {
        return frame.id.value >= minId_ && frame.id.value <= maxId_;
    }

private:
    uint32_t minId_;
    uint32_t maxId_;
};

