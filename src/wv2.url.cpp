#include "wv2.url.impl.hpp"

#include "win32.utils.hpp"
#include "win32.error.hpp"

#include <cassert>

#include <shlwapi.h>

namespace saucer
{
    url::url() : m_impl(std::move(make({.scheme = "about", .path = "blank"}).m_impl)) {}

    url::url(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    url::url(const url &other) : m_impl(std::move(parse(other.string())->m_impl)) {}

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
        return utils::narrow(m_impl->url);
    }

    fs::path url::path() const
    {
        return utils::narrow({m_impl->components.lpszUrlPath, m_impl->components.dwUrlPathLength});
    }

    std::string url::scheme() const
    {
        return utils::narrow({m_impl->components.lpszScheme, m_impl->components.dwSchemeLength});
    }

    std::optional<std::string> url::host() const
    {
        const auto *rtn = m_impl->components.lpszHostName;

        if (!rtn)
        {
            return std::nullopt;
        }

        return utils::narrow({rtn, m_impl->components.dwHostNameLength});
    }

    std::optional<std::size_t> url::port() const
    {
        return static_cast<std::size_t>(m_impl->components.nPort);
    }

    std::optional<std::string> url::user() const
    {
        const auto *rtn = m_impl->components.lpszUserName;

        if (!rtn)
        {
            return std::nullopt;
        }

        return utils::narrow({rtn, m_impl->components.dwUserNameLength});
    }

    std::optional<std::string> url::password() const
    {
        const auto *rtn = m_impl->components.lpszPassword;

        if (!rtn)
        {
            return std::nullopt;
        }

        return utils::narrow({rtn, m_impl->components.dwPasswordLength});
    }

    bool url::operator==(const url &other) const
    {
        return string() == other.string();
    }

    bool url::operator==(std::string_view other) const
    {
        return string() == other;
    }

    result<url> url::from(const fs::path &file)
    {
        std::wstring rtn{};
        DWORD len{INTERNET_MAX_URL_LENGTH};

        rtn.resize(len);

        if (auto status = UrlCreateFromPathW(file.wstring().c_str(), rtn.data(), &len, 0); !SUCCEEDED(status))
        {
            return err(status);
        }

        return parse(utils::narrow(rtn));
    }

    result<url> url::parse(cstring_view input)
    {
        if (std::string_view{input}.empty())
        {
            return url{};
        }

        auto wide = utils::widen(input);

        auto components = URL_COMPONENTSW{
            .dwStructSize     = sizeof(URL_COMPONENTSW),
            .dwSchemeLength   = 1,
            .dwHostNameLength = 1,
            .dwUserNameLength = 1,
            .dwPasswordLength = 1,
            .dwUrlPathLength  = 1,
        };

        if (InternetCrackUrlW(wide.c_str(), wide.length(), 0, &components) != TRUE)
        {
            return err(GetLastError());
        }

        return impl{.url = std::move(wide), .components = components};
    }

    url url::make(const options &opts)
    {
        auto components = URL_COMPONENTSW{.dwStructSize = sizeof(URL_COMPONENTSW)};

        auto path                  = opts.path.wstring();
        components.lpszUrlPath     = path.data();
        components.dwUrlPathLength = path.length();

        auto scheme               = utils::widen(opts.scheme);
        components.lpszScheme     = scheme.data();
        components.dwSchemeLength = scheme.length();

        std::wstring host{};

        if (opts.host.has_value())
        {
            host                        = utils::widen(*opts.host);
            components.lpszHostName     = host.data();
            components.dwHostNameLength = host.length();
        }

        if (opts.port.has_value())
        {
            components.nPort = *opts.port;
        }

        std::wstring raw{};
        DWORD len{0};

        InternetCreateUrlW(&components, 0, raw.data(), &len);
        raw.resize(len + 1);

        if (InternetCreateUrlW(&components, 0, raw.data(), &len) != TRUE)
        {
            assert(false);
        }

        auto parsed = parse(utils::narrow(raw));

        if (!parsed.has_value())
        {
            assert(false);
        }

        return unwrap_safe(parsed);
    }
} // namespace saucer
