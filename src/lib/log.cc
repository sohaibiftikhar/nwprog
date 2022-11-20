#include "src/lib/log.hh"

#include <cstring>
#include <iostream>
#include <source_location>
#include <string_view>

namespace spinscale::nwprog::log
{

namespace
{

enum class Level : uint8_t
{
  debug,
  info,
  warn,
  error
};

[[nodiscard]] std::string_view level_to_string_view(const Level level)
{
  switch (level)
  {
    case Level::debug:
      return "DEBUG";
    case Level::info:
      return "INFO";
    case Level::warn:
      return "WARN";
    case Level::error:
      return "ERROR";
  }
}

[[nodiscard]] std::ostream& level_to_stream(const Level level)
{
  switch (level)
  {
    case Level::debug:
      return std::cout;
    case Level::info:
      return std::cout;
    case Level::warn:
      return std::cerr;
    case Level::error:
      return std::cerr;
  }
}

void impl(Level level, const std::string_view message, const std::source_location& location)
{
  level_to_stream(level) << location.file_name() << "(" << location.line() << ":" << location.column() << ") "
                         << location.function_name() << "[" << level_to_string_view(level) << "]: " << message << '\n';
}

}  // namespace

void debug(const std::string_view message, const std::source_location location)
{
  impl(Level::debug, message, location);
}

void info(const std::string_view message, const std::source_location location)
{
  impl(Level::info, message, location);
}

void warn(const std::string_view message, const std::source_location location)
{
  impl(Level::warn, message, location);
}

void error(const std::string_view message, const std::source_location location)
{
  impl(Level::error, message, location);
}

void expects(const bool condition, const std::string_view message, const std::source_location location)
{
  if (!condition)
  {
    impl(Level::error, message, location);
    if (errno)
    {
      level_to_stream(Level::error) << "OS Error: " << strerror(errno) << std::endl;
    }
    exit(1);
  }
}

}  // namespace spinscale::nwprog::log
