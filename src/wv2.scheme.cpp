#include "wv2.scheme.impl.hpp"

#include "win32.utils.hpp"

namespace saucer::scheme
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : request(*other.m_impl) {}

    request::~request() = default;

    url request::url() const
    {
        utils::string_handle raw;
        m_impl->request->get_Uri(&raw.reset());
        return url::parse(utils::narrow(raw.get())).value_or({});
    }

    std::string request::method() const
    {
        utils::string_handle raw;
        m_impl->request->get_Method(&raw.reset());

        return utils::narrow(raw.get());
    }

    stash request::content() const
    {
        if (!m_impl->body)
        {
            return stash::empty();
        }

        return stash::from(utils::read(m_impl->body.Get()));
    }

    std::map<std::string, std::string> request::headers() const
    {
        ComPtr<ICoreWebView2HttpRequestHeaders> headers;
        m_impl->request->get_Headers(&headers);

        ComPtr<ICoreWebView2HttpHeadersCollectionIterator> it;
        headers->GetIterator(&it);

        std::map<std::string, std::string> rtn;
        BOOL has_header{};

        while ((it->get_HasCurrentHeader(&has_header), has_header))
        {
            utils::string_handle header;
            utils::string_handle value;

            it->GetCurrentHeader(&header.reset(), &value.reset());
            rtn.emplace(utils::narrow(header.get()), utils::narrow(value.get()));

            BOOL has_next{};
            it->MoveNext(&has_next);
        }

        return rtn;
    }
} // namespace saucer::scheme
