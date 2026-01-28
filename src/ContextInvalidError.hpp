#pragma once

#include <string>
#include <stdexcept>

class ContextInvalidError : public std::runtime_error {
public:
    explicit ContextInvalidError(const std::string& msg) : std::runtime_error(msg) {}
};