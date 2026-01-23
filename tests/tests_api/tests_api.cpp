#include "tests_api.hpp"
#include <iostream>

namespace cannect
{
  namespace test
  {

    void TestStats::record_pass()
    {
      ++total;
      ++passed;
    }
    void TestStats::record_fail()
    {
      ++total;
      ++failed;
    }
    void TestStats::print_summary() const
    {
      std::cout << "\n"
                << CANNECT_TEST_COLOR_BOLD
                << "================== Test Summary ==================" << CANNECT_TEST_COLOR_RESET << "\n";
      std::cout << "Total:  " << total << "\n";
      std::cout << CANNECT_TEST_COLOR_GREEN << "Passed: " << passed << CANNECT_TEST_COLOR_RESET << "\n";
      if (failed > 0)
      {
        std::cout << CANNECT_TEST_COLOR_RED << "Failed: " << failed << CANNECT_TEST_COLOR_RESET << "\n";
      }
      else
      {
        std::cout << "Failed: " << failed << "\n";
      }
      std::cout << CANNECT_TEST_COLOR_BOLD
                << "==================================================" << CANNECT_TEST_COLOR_RESET << "\n";
    }

    TestStats &get_test_stats()
    {
      static TestStats stats;
      return stats;
    }

    TestSuite &TestSuite::instance()
    {
      static TestSuite suite;
      return suite;
    }

    void TestSuite::add_test(const std::string &name, std::function<void()> test_func)
    {
      tests.push_back({name, test_func});
    }

    int TestSuite::run_all()
    {
      std::cout << CANNECT_TEST_COLOR_CYAN << CANNECT_TEST_COLOR_BOLD
                << "\n========== Running CANnect Tests ==========\n"
                << CANNECT_TEST_COLOR_RESET << "\n";
      for (const auto &test : tests)
      {
        std::cout << CANNECT_TEST_COLOR_BLUE << "Running: " << CANNECT_TEST_COLOR_RESET << test.name << "\n";
        try
        {
          test.test_func();
        }
        catch (const std::exception &e)
        {
          std::cout << CANNECT_TEST_COLOR_RED << "  Exception: " << e.what() << CANNECT_TEST_COLOR_RESET << "\n";
          get_test_stats().record_fail();
        }
        std::cout << "\n";
      }
      get_test_stats().print_summary();
      return get_test_stats().failed > 0 ? 1 : 0;
    }

    TestRegistrar::TestRegistrar(const std::string &name, std::function<void()> test_func)
    {
      TestSuite::instance().add_test(name, test_func);
    }

  } // namespace test
} // namespace cannect
