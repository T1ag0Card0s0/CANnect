#pragma once

#include <cassert>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ANSI color codes for terminal output
#define CANNECT_TEST_COLOR_RESET "\033[0m"
#define CANNECT_TEST_COLOR_RED "\033[31m"
#define CANNECT_TEST_COLOR_GREEN "\033[32m"
#define CANNECT_TEST_COLOR_YELLOW "\033[33m"
#define CANNECT_TEST_COLOR_BLUE "\033[34m"
#define CANNECT_TEST_COLOR_MAGENTA "\033[35m"
#define CANNECT_TEST_COLOR_CYAN "\033[36m"
#define CANNECT_TEST_COLOR_BOLD "\033[1m"

namespace cannect
{
  namespace test
  {

    struct TestStats
    {
      int total = 0;
      int passed = 0;
      int failed = 0;
      void record_pass();
      void record_fail();
      void print_summary() const;
    };

    TestStats &get_test_stats();

    struct TestCase
    {
      std::string name;
      std::function<void()> test_func;
    };

    class TestSuite
    {
    public:
      static TestSuite &instance();
      void add_test(const std::string &name, std::function<void()> test_func);
      int run_all();

    private:
      std::vector<TestCase> tests;
    };

    struct TestRegistrar
    {
      TestRegistrar(const std::string &name, std::function<void()> test_func);
    };

  } // namespace test
} // namespace cannect

#define CANNECT_TO_STRING(x)                                                                                           \
  ([&]() -> std::string {                                                                                              \
    std::ostringstream oss;                                                                                            \
    oss << x;                                                                                                          \
    return oss.str();                                                                                                  \
  }())

#define cannect_assert_equal(left, right, msg)                                                                         \
  do                                                                                                                   \
  {                                                                                                                    \
    auto _left_val = (left);                                                                                           \
    auto _right_val = (right);                                                                                         \
    bool _result = (_left_val == _right_val);                                                                          \
    if (!_result)                                                                                                      \
    {                                                                                                                  \
      std::cout << "  " << CANNECT_TEST_COLOR_RED << "✗ FAILED" << CANNECT_TEST_COLOR_RESET << " [" << __FILE__ << ":" \
                << __LINE__ << "]\n";                                                                                  \
      std::cout << "    " << msg << "\n";                                                                              \
      std::cout << "    Expected: " << CANNECT_TEST_COLOR_GREEN << _right_val << CANNECT_TEST_COLOR_RESET << "\n";     \
      std::cout << "    Actual:   " << CANNECT_TEST_COLOR_RED << _left_val << CANNECT_TEST_COLOR_RESET << "\n";        \
      cannect::test::get_test_stats().record_fail();                                                                   \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      std::cout << "  " << CANNECT_TEST_COLOR_GREEN << "✓ PASSED" << CANNECT_TEST_COLOR_RESET << " " << msg << "\n";   \
      cannect::test::get_test_stats().record_pass();                                                                   \
    }                                                                                                                  \
  } while (0)

#define cannect_assert_not_equal(left, right, msg)                                                                     \
  do                                                                                                                   \
  {                                                                                                                    \
    auto _left_val = (left);                                                                                           \
    auto _right_val = (right);                                                                                         \
    bool _result = (_left_val != _right_val);                                                                          \
    if (!_result)                                                                                                      \
    {                                                                                                                  \
      std::cout << "  " << CANNECT_TEST_COLOR_RED << "✗ FAILED" << CANNECT_TEST_COLOR_RESET << " [" << __FILE__ << ":" \
                << __LINE__ << "]\n";                                                                                  \
      std::cout << "    " << msg << "\n";                                                                              \
      std::cout << "    Expected values to be different\n";                                                            \
      std::cout << "    Both values: " << CANNECT_TEST_COLOR_RED << _left_val << CANNECT_TEST_COLOR_RESET << "\n";     \
      cannect::test::get_test_stats().record_fail();                                                                   \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      std::cout << "  " << CANNECT_TEST_COLOR_GREEN << "✓ PASSED" << CANNECT_TEST_COLOR_RESET << " " << msg << "\n";   \
      cannect::test::get_test_stats().record_pass();                                                                   \
    }                                                                                                                  \
  } while (0)

#define cannect_assert_true(condition, msg)                                                                            \
  do                                                                                                                   \
  {                                                                                                                    \
    bool _result = (condition);                                                                                        \
    if (!_result)                                                                                                      \
    {                                                                                                                  \
      std::cout << "  " << CANNECT_TEST_COLOR_RED << "✗ FAILED" << CANNECT_TEST_COLOR_RESET << " [" << __FILE__ << ":" \
                << __LINE__ << "]\n";                                                                                  \
      std::cout << "    " << msg << "\n";                                                                              \
      std::cout << "    Expected: " << CANNECT_TEST_COLOR_GREEN << "true" << CANNECT_TEST_COLOR_RESET << "\n";         \
      std::cout << "    Actual:   " << CANNECT_TEST_COLOR_RED << "false" << CANNECT_TEST_COLOR_RESET << "\n";          \
      cannect::test::get_test_stats().record_fail();                                                                   \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      std::cout << "  " << CANNECT_TEST_COLOR_GREEN << "✓ PASSED" << CANNECT_TEST_COLOR_RESET << " " << msg << "\n";   \
      cannect::test::get_test_stats().record_pass();                                                                   \
    }                                                                                                                  \
  } while (0)

#define cannect_assert_false(condition, msg)                                                                           \
  do                                                                                                                   \
  {                                                                                                                    \
    bool _result = !(condition);                                                                                       \
    if (!_result)                                                                                                      \
    {                                                                                                                  \
      std::cout << "  " << CANNECT_TEST_COLOR_RED << "✗ FAILED" << CANNECT_TEST_COLOR_RESET << " [" << __FILE__ << ":" \
                << __LINE__ << "]\n";                                                                                  \
      std::cout << "    " << msg << "\n";                                                                              \
      std::cout << "    Expected: " << CANNECT_TEST_COLOR_GREEN << "false" << CANNECT_TEST_COLOR_RESET << "\n";        \
      std::cout << "    Actual:   " << CANNECT_TEST_COLOR_RED << "true" << CANNECT_TEST_COLOR_RESET << "\n";           \
      cannect::test::get_test_stats().record_fail();                                                                   \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      std::cout << "  " << CANNECT_TEST_COLOR_GREEN << "✓ PASSED" << CANNECT_TEST_COLOR_RESET << " " << msg << "\n";   \
      cannect::test::get_test_stats().record_pass();                                                                   \
    }                                                                                                                  \
  } while (0)

#define cannect_assert_null(ptr, msg)                                                                                  \
  do                                                                                                                   \
  {                                                                                                                    \
    auto _ptr_val = (ptr);                                                                                             \
    bool _result = (_ptr_val == nullptr);                                                                              \
    if (!_result)                                                                                                      \
    {                                                                                                                  \
      std::cout << "  " << CANNECT_TEST_COLOR_RED << "✗ FAILED" << CANNECT_TEST_COLOR_RESET << " [" << __FILE__ << ":" \
                << __LINE__ << "]\n";                                                                                  \
      std::cout << "    " << msg << "\n";                                                                              \
      std::cout << "    Expected: " << CANNECT_TEST_COLOR_GREEN << "nullptr" << CANNECT_TEST_COLOR_RESET << "\n";      \
      std::cout << "    Actual:   " << CANNECT_TEST_COLOR_RED << _ptr_val << CANNECT_TEST_COLOR_RESET << "\n";         \
      cannect::test::get_test_stats().record_fail();                                                                   \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      std::cout << "  " << CANNECT_TEST_COLOR_GREEN << "✓ PASSED" << CANNECT_TEST_COLOR_RESET << " " << msg << "\n";   \
      cannect::test::get_test_stats().record_pass();                                                                   \
    }                                                                                                                  \
  } while (0)

#define cannect_assert_not_null(ptr, msg)                                                                              \
  do                                                                                                                   \
  {                                                                                                                    \
    auto _ptr_val = (ptr);                                                                                             \
    bool _result = (_ptr_val != nullptr);                                                                              \
    if (!_result)                                                                                                      \
    {                                                                                                                  \
      std::cout << "  " << CANNECT_TEST_COLOR_RED << "✗ FAILED" << CANNECT_TEST_COLOR_RESET << " [" << __FILE__ << ":" \
                << __LINE__ << "]\n";                                                                                  \
      std::cout << "    " << msg << "\n";                                                                              \
      std::cout << "    Expected: non-null pointer\n";                                                                 \
      std::cout << "    Actual:   " << CANNECT_TEST_COLOR_RED << "nullptr" << CANNECT_TEST_COLOR_RESET << "\n";        \
      cannect::test::get_test_stats().record_fail();                                                                   \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
      std::cout << "  " << CANNECT_TEST_COLOR_GREEN << "✓ PASSED" << CANNECT_TEST_COLOR_RESET << " " << msg << "\n";   \
      cannect::test::get_test_stats().record_pass();                                                                   \
    }                                                                                                                  \
  } while (0)

#define CANNECT_TEST_CASE(name)                                                                                        \
  void name##_test_func();                                                                                             \
  static cannect::test::TestRegistrar name##_registrar(#name, name##_test_func);                                       \
  void name##_test_func()

#define CANNECT_RUN_ALL_TESTS() cannect::test::TestSuite::instance().run_all()
