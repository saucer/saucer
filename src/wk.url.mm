#include "wk.url.impl.hpp"

#include "error.impl.hpp"
#include "cocoa.utils.hpp"

namespace saucer
{
    url::url() : m_impl(std::move(make({}).m_impl)) {}

    url::url.impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    url::url(const url const url &other) : url(*other.m_impl) {}

    url::url(url &&other) noexcept : url()
    {
        swap(*this, other);
    }

    url::~url() = default;

    url &url::operator=(url other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    void swap(url &first, url &second) noexcept
    {
        using std::swap;
        swap(first.m_impl, second.m_impl);
    }

    std::string url::string() const
    {
        const utils::autorelease_guard guard{};
        return m_impl->url.get().absoluteString.UTF8String;
    }

    fs::path url::path() const
    {
        const utils::autorelease_guard guard{};
        const auto *rtn = m_impl->url.get().path;

        if (!rtn)
        {
            return {};
        }

        return rtn.UTF8String;
    }

    std::string url::scheme() const
    {
        const utils::autorelease_guard guard{};
        const auto *rtn = m_impl->url.get().scheme;

        if (!rtn)
        {
            return {};
        }

        return rtn.UTF8String;
    }

    std::optional<std::string> url::host() const
    {
        const utils::autorelease_guard guard{};
        const auto *rtn = m_impl->url.get().host;

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn.UTF8String;
    }

    std::optional<std::size_t> url::port() const
    {
        const utils::autorelease_guard guard{};
        const auto *rtn = m_impl->url.get().port;

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn.unsignedLongValue;
    }

    std::optional<std::string> url::user() const
    {
        const utils::autorelease_guard guard{};
        const auto *rtn = m_impl->url.get().user;

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn.UTF8String;
    }

    std::optional<std::string> url::password() const
    {
        const utils::autorelease_guard guard{};
        const auto *rtn = m_impl->url.get().password;

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn.UTF8String;
    }

    result<url> url::from(const fs::path &file)
    {
        const utils::autorelease_guard guard{};
        auto *const rtn = [NSURL fileURLWithPath:[NSString stringWithUTF8String:file.c_str()]];

        if (!rtn)
        {
            return err(std::error_code{});
        }

        return impl{utils::objc_ptr<NSURL>::ref(rtn)};
    }

    result<url> url::parse(const std::string &input)
    {
        const utils::autorelease_guard guard{};
        auto *const rtn = [NSURL URLWithString:[NSString stringWithUTF8String:input.c_str()]];

        if (!rtn)
        {
            return err(std::error_code{});
        }

        return impl{utils::objc_ptr<NSURL>::ref(rtn)};
    }

    url url::make(const options &opts)
    {
        const utils::autorelease_guard guard{};
        auto *const rtn = [[[NSURLComponents alloc] init] autorelease];

        if (!opts.scheme.empty())
        {
            [rtn setScheme:[NSString stringWithUTF8String:opts.scheme.c_str()]];
        }

        if (opts.host.has_value())
        {
            [rtn setHost:[NSString stringWithUTF8String:opts.host.value().c_str()]];
        }

        if (opts.port.has_value())
        {
            [rtn setPort:[NSNumber numberWithUnsignedLong:opts.port.value()]];
        }

        [rtn setPath:[NSString stringWithUTF8String:opts.path.c_str()]];

        return impl{[rtn.URL copy]};
    }
} // namespace saucer
