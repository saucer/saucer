#include "wv2.uri.impl.hpp"

#include "win32.utils.hpp"

#include <cassert>

#include <shlwapi.h>

namespace saucer
{
    uri::uri() : m_impl(std::move(make({}).m_impl)) {}

    uri::uri(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    uri::uri(const uri &other) : m_impl(std::move(parse(other.string())->m_impl)) {}

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
        return utils::narrow(m_impl->url);
    }

    fs::path uri::path() const
    {
        return utils::narrow({m_impl->components.lpszUrlPath, m_impl->components.dwUrlPathLength});
    }

    std::string uri::scheme() const
    {
        return utils::narrow({m_impl->components.lpszScheme, m_impl->components.dwSchemeLength});
    }

    std::optional<std::string> uri::host() const
    {
        const auto *rtn = m_impl->components.lpszHostName;

        if (!rtn)
        {
            return std::nullopt;
        }

        return utils::narrow({rtn, m_impl->components.dwHostNameLength});
    }

    std::optional<std::size_t> uri::port() const
    {
        return static_cast<std::size_t>(m_impl->components.nPort);
    }

    std::optional<std::string> uri::user() const
    {
        const auto *rtn = m_impl->components.lpszUserName;

        if (!rtn)
        {
            return std::nullopt;
        }

        return utils::narrow({rtn, m_impl->components.dwUserNameLength});
    }

    std::optional<std::string> uri::password() const
    {
        const auto *rtn = m_impl->components.lpszPassword;

        if (!rtn)
        {
            return std::nullopt;
        }

        return utils::narrow({rtn, m_impl->components.dwPasswordLength});
    }

    std::optional<uri> uri::from(const fs::path &file)
    {
        std::wstring rtn{};
        DWORD len{INTERNET_MAX_URL_LENGTH};

        rtn.resize(len);

        if (!SUCCEEDED(UrlCreateFromPathW(file.wstring().c_str(), rtn.data(), &len, NULL)))
        {
            return std::nullopt;
        }

        return parse(utils::narrow(rtn));
    }

    std::optional<uri> uri::parse(const std::string &input)
    {
        auto wide = utils::widen(input);

        auto components = URL_COMPONENTSW{
            .dwStructSize     = sizeof(URL_COMPONENTSW),
            .dwSchemeLength   = 1,
            .dwHostNameLength = 1,
            .dwUserNameLength = 1,
            .dwPasswordLength = 1,
            .dwUrlPathLength  = 1,
        };

        if (InternetCrackUrlW(wide.c_str(), wide.length(), NULL, &components) != TRUE)
        {
            return std::nullopt;
        }

        return impl{.url = std::move(wide), .components = components};
    }

    uri uri::make(const options &opts)
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
            host                        = utils::widen(opts.host.value());
            components.lpszHostName     = host.data();
            components.dwHostNameLength = host.length();
        }

        if (opts.port)
        {
            components.nPort = opts.port.value();
        }

        std::wstring url{};
        DWORD len{0};

        InternetCreateUrlW(&components, NULL, url.data(), &len);
        url.resize(len + 1);

        if (InternetCreateUrlW(&components, NULL, url.data(), &len) != TRUE)
        {
            assert(false && "Failed to create URI");
        }

        auto parsed = parse(utils::narrow(url));

        if (!parsed.has_value())
        {
            assert(false && "Failed to re-parse URL");
            return impl{.url = std::move(url)};
        }

        return parsed.value();
    }
} // namespace saucer
