#include "cannect/Cannect.hpp"
#include "cannect/cli/ArgumentParser.hpp"
#include "cannect/cli/Logger.hpp"
#include "cannect/core/SocketCanTransport.hpp"

#include "tests_api.hpp"

using namespace cannect;

CANNECT_TEST_CASE(Cannect_Construct)
{
  Cannect app;
  // Just construct to cover constructor/destructor
}

CANNECT_TEST_CASE(ArgumentParser_Construct)
{
  ArgumentParser parser("test", "0.0.1");
  parser.addArgument("--foo", ArgType::STRING, "foo arg");
}

CANNECT_TEST_CASE(Logger_GetInstance)
{
  Logger *logger = Logger::getInstance();
  logger->log("Logger coverage test");
}

CANNECT_TEST_CASE(SocketCanTransport_IsOpen_Default)
{
  cannect::SocketCanTransport sock;
  cannect_assert_false(sock.isOpen(), "Default SocketCanTransport should not be open");
}

CANNECT_TEST_CASE(BasicArithmetic)
{
  cannect_assert_equal(1 + 1, 2, "Addition works correctly");
  cannect_assert_equal(5 * 2, 10, "Multiplication works correctly");
  cannect_assert_not_equal(3, 4, "Different values are not equal");
}

CANNECT_TEST_CASE(BooleanTests)
{
  cannect_assert_true(true, "True is true");
  cannect_assert_false(false, "False is false");
  cannect_assert_true(5 > 3, "5 is greater than 3");
}

CANNECT_TEST_CASE(PointerTests)
{
  int *null_ptr = nullptr;
  int value = 42;
  int *valid_ptr = &value;

  cannect_assert_null(null_ptr, "Null pointer is null");
  cannect_assert_not_null(valid_ptr, "Valid pointer is not null");
}

int main()
{
  return CANNECT_RUN_ALL_TESTS();
}
