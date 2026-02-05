#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

namespace cannect
{

  enum class ArgType
  {
    NONE,
    BOOL,
    STRING,
    INT,
    FLOAT
  };
  struct Argument
  {
    std::string name;
    std::string help;
    ArgType type;
    std::optional<std::variant<std::string, int, float>> value;
    bool present = false;
  };

  class ArgumentParser
  {
  public:
    ArgumentParser(const std::string &name, const std::string &version);

    void addArgument(const std::string &name, ArgType type, const std::string &help);

    bool parse(int argc, char *argv[]);

    bool has(const std::string &name) const;

    std::string getString(const std::string &name) const;

    int getInt(const std::string &name) const;

    float getFloat(const std::string &name) const;

    void help() const;

  private:
    bool parseValue(Argument &arg, const std::string &valueStr);

    std::unordered_map<std::string, Argument> arguments;
    std::string name;
    std::string version;
  };

} // namespace cannect
