#include "wv2.scheme.impl.hpp"

#include "win32.utils.hpp"

namespace saucer::scheme
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::~request() = default;

    std::string request::url() const
    {
        LPWSTR raw{};
        m_impl->request->get_Uri(&raw);

        auto rtn = utils::narrow(raw);
        CoTaskMemFree(raw);

        return rtn;
    }

    std::string request::method() const
    {
        LPWSTR raw{};
        m_impl->request->get_Method(&raw);

        auto rtn = utils::narrow(raw);
        CoTaskMemFree(raw);

        return rtn;
    }

    stash<> request::content() const
    {
        if (!m_impl->body)
        {
            return stash<>::empty();
        }

        return stash<>::from(utils::read(m_impl->body.Get()));
    }

    std::map<std::string, std::string> request::headers() const
    {
        ComPtr<ICoreWebView2HttpRequestHeaders> headers;
        m_impl->request->get_Headers(&headers);

        ComPtr<ICoreWebView2HttpHeadersCollectionIterator> it;
        headers->GetIterator(&it);

        std::map<std::string, std::string> rtn;
        BOOL empty{};

        while ((it->get_HasCurrentHeader(&empty), empty))
        {
            LPWSTR header{};
            LPWSTR value{};

            it->GetCurrentHeader(&header, &value);
            rtn.emplace(utils::narrow(header), utils::narrow(value));

            CoTaskMemFree(header);
            CoTaskMemFree(value);

            BOOL has_next{};
            it->MoveNext(&has_next);
        }

        return rtn;
    }
} // namespace saucer::scheme
