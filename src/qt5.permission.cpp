#include "qt.permission.impl.hpp"

namespace saucer::permission
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {} // NOLINT(*-move-const-arg)

    request::~request() = default;

    uri request::url() const // NOLINT(*-static)
    {
        return {};
    }

    permission::type request::type() const // NOLINT(*-static)
    {
        return {};
    }

    void request::accept(bool) const {}
} // namespace saucer::permission
