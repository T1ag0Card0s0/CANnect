#pragma once
#include <span>
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>


using InterfaceId = std::string;
using Clock = std::chrono::steady_clock;
using Timestamp = Clock::time_point;

enum class FrameFormat {
    Standard,   // 11-bit ID
    Extended    // 29-bit ID
};

enum class FrameType {
    Data,
    Remote,
    Error
};

struct CanId {
    uint32_t value{0};
    FrameFormat format{FrameFormat::Standard};

    [[nodiscard]] bool isValid() const noexcept {
        if (format == FrameFormat::Standard) {
            return value <= 0x7FF;
        }
        return value <= 0x1FFFFFFF;
    }
};

struct CanFrame {
    CanId id{};
    FrameType type{FrameType::Data};
    std::array<uint8_t, 64> payload{};   // supports CAN FD too
    uint8_t size{0};
    bool isFd{false};
    bool bitrateSwitch{false};
    bool esi{false};
    Timestamp timestamp{};
    std::optional<InterfaceId> sourceInterface{};

    [[nodiscard]] std::span<const uint8_t> data() const noexcept {
        return std::span<const uint8_t>(payload.data(), size);
    }
};

