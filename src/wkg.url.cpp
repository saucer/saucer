#include "wkg.url.impl.hpp"

#include "gtk.utils.hpp"
#include "gtk.error.hpp"

namespace saucer
{
    url::url() : m_impl(std::move(make({}).m_impl)) {}

    url::url(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    url::url(const url &other) : url(*other.m_impl) {}

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
        return std::string{utils::g_str_ptr{g_uri_to_string(m_impl->uri.get())}.get()};
    }

    fs::path url::path() const
    {
        return g_uri_get_path(m_impl->uri.get());
    }

    std::string url::scheme() const
    {
        return g_uri_get_scheme(m_impl->uri.get());
    }

    std::optional<std::string> url::host() const
    {
        const auto *rtn = g_uri_get_host(m_impl->uri.get());

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn;
    }

    std::optional<std::size_t> url::port() const
    {
        const auto rtn = g_uri_get_port(m_impl->uri.get());

        if (rtn == -1)
        {
            return std::nullopt;
        }

        return static_cast<std::size_t>(rtn);
    }

    std::optional<std::string> url::user() const
    {
        const auto *rtn = g_uri_get_user(m_impl->uri.get());

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn;
    }

    std::optional<std::string> url::password() const
    {
        const auto *rtn = g_uri_get_password(m_impl->uri.get());

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn;
    }

    result<url> url::from(const fs::path &file)
    {
        auto ec   = std::error_code{};
        auto path = fs::canonical(file, ec);

        if (ec)
        {
            return err(ec);
        }

        auto error      = utils::g_error_ptr{};
        const auto *rtn = g_filename_to_uri(path.c_str(), nullptr, &error.reset());

        if (!rtn)
        {
            return err(std::move(error));
        }

        return parse(rtn);
    }

    result<url> url::parse(cstring_view input)
    {
        if (std::string_view{input}.empty())
        {
            return url{};
        }

        auto error      = utils::g_error_ptr{};
        auto *const rtn = g_uri_parse(input.c_str(), G_URI_FLAGS_NONE, &error.reset());

        if (!rtn)
        {
            return err(std::move(error));
        }

        return impl{rtn};
    }

    url url::make(const options &opts)
    {
        return impl{
            g_uri_build(G_URI_FLAGS_NONE,                                                               //
                        opts.scheme.c_str(),                                                            //
                        nullptr,                                                                        //
                        opts.host.transform(&std::string::c_str).value_or(nullptr),                     //
                        opts.port.transform([](auto &&x) { return static_cast<int>(x); }).value_or(-1), //
                        opts.path.c_str(),                                                              //
                        nullptr,                                                                        //
                        nullptr                                                                         //
                        ),
        };
    }
} // namespace saucer
