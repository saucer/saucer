#include "wk.scheme.impl.hpp"

#include "wk.url.impl.hpp"

namespace saucer::scheme
{
    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : request(*other.m_impl) {}

    request::~request() = default;

    url request::url() const
    {
        return url::impl{[m_impl->task.get().request.URL copy]};
    }

    std::string request::method() const
    {
        return m_impl->task.get().request.HTTPMethod.UTF8String;
    }

    stash request::content() const
    {
        auto *const body = m_impl->task.get().request.HTTPBody;

        if (!body)
        {
            return stash::empty();
        }

        const auto *raw = reinterpret_cast<const std::uint8_t *>(body.bytes);
        return stash::from({raw, raw + body.length});
    }

    std::map<std::string, std::string> request::headers() const
    {
        auto *const headers = m_impl->task.get().request.allHTTPHeaderFields;

        std::map<std::string, std::string> rtn;

        [headers enumerateKeysAndObjectsUsingBlock:[&rtn](NSString *key, NSString *value, BOOL *)
                 {
                     rtn.emplace(key.UTF8String, value.UTF8String);
                 }];

        return rtn;
    }
} // namespace saucer::scheme
