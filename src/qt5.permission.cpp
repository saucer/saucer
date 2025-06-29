#include "qt.permission.impl.hpp"

namespace saucer::permission
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : request(*other.m_impl) {}

    request::~request() = default;

    uri request::url() const // NOLINT(*-static)
    {
        return {};
    }

    permission::type request::type() const // NOLINT(*-static)
    {
        return {};
    }

    void request::accept([[maybe_unused]] bool value) const // NOLINT(*-static)
    {
    }
} // namespace saucer::permission
