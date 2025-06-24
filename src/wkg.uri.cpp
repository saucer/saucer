#include "wkg.uri.impl.hpp"

#include "gtk.utils.hpp"

#include <cassert>

namespace saucer
{
    uri::uri() : m_impl(std::move(make({}).m_impl)) {}

    uri::uri(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    uri::uri(const uri &other) : m_impl(std::make_unique<impl>(*other.m_impl)) {}

    uri::uri(uri &&other) noexcept : uri()
    {
        std::swap(m_impl, other.m_impl);
    }

    uri::~uri() = default;

    uri &uri::operator=(const uri &other)
    {
        if (this != &other)
        {
            m_impl = std::make_unique<impl>(*other.m_impl);
        }

        return *this;
    }

    uri &uri::operator=(uri &&other) noexcept
    {
        if (this != &other)
        {
            std::swap(m_impl, other.m_impl);
        }

        return *this;
    }

    std::string uri::string() const
    {
        return std::string{utils::g_str_ptr{g_uri_to_string(m_impl->uri.get())}.get()};
    }

    std::optional<std::string> uri::host() const
    {
        const auto *rtn = g_uri_get_host(m_impl->uri.get());

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn;
    }

    std::optional<std::string> uri::query() const
    {
        const auto *rtn = g_uri_get_query(m_impl->uri.get());

        if (!rtn)
        {
            return std::nullopt;
        }

        return rtn;
    }

    fs::path uri::path() const
    {
        return g_uri_get_path(m_impl->uri.get());
    }

    std::string uri::scheme() const
    {
        return g_uri_get_scheme(m_impl->uri.get());
    }

    std::optional<std::size_t> uri::port() const
    {
        const auto rtn = g_uri_get_port(m_impl->uri.get());

        if (rtn == -1)
        {
            return std::nullopt;
        }

        return static_cast<std::size_t>(rtn);
    }

    std::optional<uri> uri::from(const fs::path &file)
    {
        const auto *rtn = g_filename_to_uri(file.c_str(), nullptr, nullptr);

        if (!rtn)
        {
            return std::nullopt;
        }

        return parse(rtn);
    }

    std::optional<uri> uri::parse(const std::string &input)
    {
        auto *const rtn = g_uri_parse(input.c_str(), G_URI_FLAGS_NONE, nullptr);

        if (!rtn)
        {
            return std::nullopt;
        }

        return impl{rtn};
    }

    uri uri::make(const options &opts)
    {
        return impl{
            g_uri_build(G_URI_FLAGS_NONE,                                                               //
                        opts.scheme.c_str(),                                                            //
                        nullptr,                                                                        //
                        opts.host.transform(&std::string::c_str).value_or(nullptr),                     //
                        opts.port.transform([](auto &&x) { return static_cast<int>(x); }).value_or(-1), //
                        opts.path.c_str(),                                                              //
                        opts.query.transform(&std::string::c_str).value_or(nullptr),                    //
                        nullptr                                                                         //
                        ),
        };
    }
} // namespace saucer
