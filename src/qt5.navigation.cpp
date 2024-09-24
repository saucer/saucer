#include "qt.navigation.impl.hpp"

namespace saucer
{
    navigation::navigation(impl data) : m_impl(std::make_unique<impl>(data)) {}

    navigation::navigation(const navigation &other) : m_impl(std::make_unique<impl>(*other.m_impl)) {}

    navigation::navigation(navigation &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    navigation::~navigation() = default;

    std::string navigation::url() const // NOLINT(*-static)
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
