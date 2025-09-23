#include "wkg.permission.impl.hpp"

namespace saucer::permission
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::~request()
    {
        accept(false);
    }

    url request::url() const
    {
        return m_impl->url;
    }

    permission::type request::type() const
    {
        return m_impl->type;
    }

    void request::accept(bool value) const
    {
        if (!m_impl->request)
        {
            return;
        }

        auto request = std::move(m_impl->request);

        (value ? webkit_permission_request_allow : webkit_permission_request_deny)(request.get());
    }
} // namespace saucer::permission
