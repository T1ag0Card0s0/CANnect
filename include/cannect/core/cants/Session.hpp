#pragma once

enum class SessionType
{
  SET_BLOCK,
  GET_BLOCK
};

enum class SessionState
{
  IDLE,
  REQUESTED,
  RECEIVING,
  AWAITING_RETRANSMIT,
  COMPLETE,
  ABORTED
};

class Session
{
public:
  Session() = default;
  ~Session() = default;

private:
  SessionState state;
};
