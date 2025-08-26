#include "error.impl.hpp"

#include <utility>

namespace saucer
{
    error::error() : m_impl(impl{}) {}

    error::error(cr::polo<impl> data, std::source_location location) : m_impl(std::move(data))
    {
        m_impl->location = location;
    }

    error::error(const error &) = default;

    error::error(error &&) noexcept = default;

    error::~error() = default;

    error &error::operator=(const error &) = default;

    error &error::operator=(error &&) noexcept = default;

    int error::code() const
    {
        return m_impl->code();
    }

    error::category error::type() const
    {
        return m_impl->type();
    }

    std::string error::message() const
    {
        return m_impl->message();
    }

    std::source_location error::location() const
    {
        return m_impl->location;
    }
} // namespace saucer
