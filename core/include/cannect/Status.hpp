#pragma once

#include <string>

namespace can {

enum class ErrorCode {
    None,
    InvalidArgument,
    NotInitialized,
    AlreadyRunning,
    IoError,
    Timeout,
    Unsupported,
    Disconnected,
    InternalError
};

class Status {
public:
    Status() = default;
    Status(ErrorCode code, std::string message = {})
        : code_(code), message_(std::move(message)) {}

    [[nodiscard]] bool ok() const noexcept { return code_ == ErrorCode::None; }
    [[nodiscard]] ErrorCode code() const noexcept { return code_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }

    static Status Ok() { return {}; }

private:
    ErrorCode code_{ErrorCode::None};
    std::string message_{};
};

template<typename T>
class Result {
public:
    Result(T value) : value_(std::move(value)), status_(Status::Ok()) {}
    Result(Status status) : value_(), status_(std::move(status)) {}

    [[nodiscard]] bool ok() const noexcept { return status_.ok(); }
    [[nodiscard]] const Status& status() const noexcept { return status_; }
    [[nodiscard]] T& value() noexcept { return *value_; }
    [[nodiscard]] const T& value() const noexcept { return *value_; }

private:
    std::optional<T> value_{};
    Status status_{};
};

} // namespace can
