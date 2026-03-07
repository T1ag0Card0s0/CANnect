#pragma once

#define STATUS_CODE_LIST                                                                                               \
    X(SUCCESS, 0, "Success")                                                                                           \
    X(UNSUCCESS, 1, "Unsuccess")                                                                                       \
    X(INVALID_ARGUMENT, 2, "Invalid argument")                                                                         \
    X(INTERFACE_ALREADY_OPEN, 3, "Interface already open")                                                             \
    X(INTERFACE_INVALID_NAME, 4, "Interface name is invalid")

enum class Status
{
#define X(name, value, desc) name = value,
    STATUS_CODE_LIST
#undef X
};

inline const char *statusToString(Status code)
{
    switch (code)
    {
#define X(name, value, desc)                                                                                           \
    case Status::name:                                                                                                 \
        return #name;
        STATUS_CODE_LIST
#undef X
    default:
        return "UNKNOWN_ERROR";
    }
}

inline const char *statusDescription(Status code)
{
    switch (code)
    {
#define X(name, value, desc)                                                                                           \
    case Status::name:                                                                                                 \
        return desc;
        STATUS_CODE_LIST
#undef X
    default:
        return "Unknown error";
    }
}
