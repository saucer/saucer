#include "serializers/errors/bad_function.hpp"

#include <fmt/core.h>

namespace saucer::errors
{
    bad_function::~bad_function() = default;

    bad_function::bad_function(std::string function) : m_function(std::move(function)) {}

    std::string bad_function::what()
    {
        return fmt::format("Unknown function \"{}\", was it exported?", m_function);
    }
} // namespace saucer::errors
