#include "qt.navigation.impl.hpp"

namespace saucer
{
    navigation::navigation(impl data) : m_impl(std::make_unique<impl>(data)) {}

    navigation::navigation(const navigation &other) : navigation(*other.m_impl) {}

    navigation::~navigation() = default;

    uri navigation::url() const // NOLINT(*-static)
    {
        return {};
    }

    bool navigation::new_window() const // NOLINT(*-static)
    {
        return {};
    }

    bool navigation::redirection() const // NOLINT(*-static)
    {
        return {};
    }

    bool navigation::user_initiated() const // NOLINT(*-static)
    {
        return {};
    }
} // namespace saucer
