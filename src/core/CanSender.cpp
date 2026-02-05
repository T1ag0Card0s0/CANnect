#include "cannect/core/CanSender.hpp"

using namespace cannect;

int CanSender::sendFrame(const CanFrame &frame)
{
  return transport->writeFrame(frame);
}
