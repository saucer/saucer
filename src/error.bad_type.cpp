#include "serializers/errors/bad_type.hpp"

#include <fmt/core.h>

namespace saucer::errors
{
    bad_type::~bad_type() = default;

    bad_type::bad_type(std::size_t index, std::string expected) : m_index(index), m_expected(std::move(expected)) {}

    std::string bad_type::what()
    {
        return fmt::format("Expected argument at position {} to be \"{}\"", m_index, m_expected);
    }
} // namespace saucer::errors
