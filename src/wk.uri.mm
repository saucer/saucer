#include "wk.uri.impl.hpp"

#include "error.impl.hpp"
#include "cocoa.utils.hpp"

namespace saucer
{
    uri::uri() : m_impl(std::make_unique<impl>([[NSURL alloc] init])) {}

    uri::uri(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    uri::uri(const uri &other) : uri(*other.m_impl) {}

    uri::uri(uri &&other) noexcept : uri()
    {
        swap(*this, other);
    }

    uri::~uri() = default;

    uri &uri::operator=(uri other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    void swap(uri &first, uri &second) noexcept
    {
        using std::swap;
        swap(first.m_impl, second.m_impl);
    }

    std::string uri::string() const
    {
        const utils::autorelease_guard guard{};
        return m_impl->url.get().absoluteString.UTF8String;
    }

    fs::path uri::path() const
    {
        const utils::autorelease_guard guard{};
        const auto *rtn = m_impl->url.get().path;

        if (!rtn)
        {
            return {};
        }

        return rtn.UTF8String;
    }

    std::string uri::scheme() const
    {
        const utils::autorelease_guard guard{};
        const auto *rtn = m_impl->url.get().scheme;

        if (!rtn)
        {
            return {};
        }

        return rtn.UTF8String;
    }

    std::optional<std::string> uri::host() const
    {
        const utils::autorelease_guard guard{};
        const auto *rtn = m_impl->url.get().host;

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn.UTF8String;
    }

    std::optional<std::size_t> uri::port() const
    {
        const utils::autorelease_guard guard{};
        const auto *rtn = m_impl->url.get().port;

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn.unsignedLongValue;
    }

    std::optional<std::string> uri::user() const
    {
        const utils::autorelease_guard guard{};
        const auto *rtn = m_impl->url.get().user;

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn.UTF8String;
    }

    std::optional<std::string> uri::password() const
    {
        const utils::autorelease_guard guard{};
        const auto *rtn = m_impl->url.get().password;

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn.UTF8String;
    }

    result<uri> uri::from(const fs::path &file)
    {
        const utils::autorelease_guard guard{};
        auto *const rtn = [NSURL fileURLWithPath:[NSString stringWithUTF8String:file.c_str()]];

        if (!rtn)
        {
            return err(std::error_code{});
        }

        return impl{utils::objc_ptr<NSURL>::ref(rtn)};
    }

    result<uri> uri::parse(const std::string &input)
    {
        const utils::autorelease_guard guard{};
        auto *const rtn = [NSURL URLWithString:[NSString stringWithUTF8String:input.c_str()]];

        if (!rtn)
        {
            return err(std::error_code{});
        }

        return impl{utils::objc_ptr<NSURL>::ref(rtn)};
    }

    uri uri::make(const options &opts)
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
