#include "cannect/cli/ArgumentParser.hpp"

using namespace cannect;

ArgumentParser::ArgumentParser(const std::string &name, const std::string &version) : name(name), version(version)
{
}

void ArgumentParser::addArgument(const std::string &name, ArgType type, const std::string &help)
{
  arguments[name] = {name, help, type, std::nullopt, false};
}

bool ArgumentParser::parse(int argc, char *argv[])
{
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help")
    {
      help();
      return false;
    }

    if (arg == "--version" || arg == "-v")
    {
      std::cout << name << " version " << version << std::endl;
      return false;
    }

    auto it = arguments.find(arg);
    if (it == arguments.end())
    {
      std::cerr << "Unknown argument: " << arg << std::endl;
      return false;
    }

    it->second.present = true;

    if (it->second.type != ArgType::NONE)
    {
      if (i + 1 >= argc)
      {
        std::cerr << "Argument " << arg << " expects a value" << std::endl;
        return false;
      }

      std::string value = argv[++i];
      if (!parseValue(it->second, value))
      {
        std::cerr << "Invalid value for " << arg << ": " << value << std::endl;
        return false;
      }
    }
  }
  return true;
}

bool ArgumentParser::has(const std::string &name) const
{
  auto it = arguments.find(name);
  return it != arguments.end() && it->second.present;
}

std::string ArgumentParser::getString(const std::string &name) const
{
  auto it = arguments.find(name);
  if (it != arguments.end() && it->second.value.has_value())
  {
    return std::get<std::string>(it->second.value.value());
  }
  return "";
}

int ArgumentParser::getInt(const std::string &name) const
{
  auto it = arguments.find(name);
  if (it != arguments.end() && it->second.value.has_value())
  {
    return std::get<int>(it->second.value.value());
  }
  return 0;
}

float ArgumentParser::getFloat(const std::string &name) const
{
  auto it = arguments.find(name);
  if (it != arguments.end() && it->second.value.has_value())
  {
    return std::get<float>(it->second.value.value());
  }
  return 0.0f;
}

void ArgumentParser::help() const
{
  std::cout << name << " " << version << std::endl;
  std::cout << "Usage: " << name << " [options]\n\nOptions:\n";
  for (const auto &[name, arg] : arguments)
  {
    std::cout << "  " << name << "\t";
    switch (arg.type)
    {
    case ArgType::NONE:
      std::cout << "       ";
      break;
    case ArgType::STRING:
      std::cout << "<str> ";
      break;
    case ArgType::INT:
      std::cout << "<int> ";
      break;
    case ArgType::FLOAT:
      std::cout << "<float>";
      break;
    default:
      break;
    }
    std::cout << "\t" << arg.help << std::endl;
  }
  std::cout << "  -h, --help\t       \tShow this help message" << std::endl;
}

bool ArgumentParser::parseValue(Argument &arg, const std::string &valueStr)
{
  try
  {
    switch (arg.type)
    {
    case ArgType::STRING:
      arg.value = valueStr;
      return true;

    case ArgType::INT: {
      size_t pos;
      int val = std::stoi(valueStr, &pos);
      if (pos != valueStr.length())
        return false;
      arg.value = val;
      return true;
    }

    case ArgType::FLOAT: {
      size_t pos;
      float val = std::stof(valueStr, &pos);
      if (pos != valueStr.length())
        return false;
      arg.value = val;
      return true;
    }

    case ArgType::NONE:
      return true;

    case ArgType::BOOL:
      if (valueStr == "true" || valueStr == "1")
      {
        arg.value = std::string("true");
        return true;
      }
      else if (valueStr == "false" || valueStr == "0")
      {
        arg.value = std::string("false");
        return true;
      }
      return true;
    }
  }
  catch (...)
  {
    return false;
  }
  return false;
}
