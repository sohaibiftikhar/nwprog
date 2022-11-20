#pragma once

#include <source_location>
#include <string_view>

namespace spinscale::nwprog::log
{

/// Logging helpers.
void debug(const std::string_view message, const std::source_location location = std::source_location::current());
void info(const std::string_view message, const std::source_location location = std::source_location::current());
void warn(const std::string_view message, const std::source_location location = std::source_location::current());
void error(const std::string_view message, const std::source_location location = std::source_location::current());

/// Expect helper. Terminates if condition is not met.
void expects(
  const bool condition, const std::string_view message,
  const std::source_location location = std::source_location::current());

}  // namespace spinscale::nwprog::lib::log
