#include "wkg.permission.impl.hpp"

namespace saucer::permission
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : m_impl(std::make_unique<impl>(*other.m_impl)) {}

    request::request(request &&other) noexcept : m_impl(std::move(other.m_impl)) {}

    request::~request() = default;

    std::string request::url() const
    {
        return m_impl->url;
    }

    permission::type request::type() const
    {
        return m_impl->type;
    }

    void request::accept(bool value) const
    {
        (value ? webkit_permission_request_allow : webkit_permission_request_deny)(m_impl->request.get());
    }
} // namespace saucer::permission
