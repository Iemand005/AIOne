#pragma once

#include <stdexcept>
#include <string>

class ContextInvalidError : public std::runtime_error {
 public:
  explicit ContextInvalidError(const std::string& msg) : std::runtime_error(msg) {}
};