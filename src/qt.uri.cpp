#include "qt.uri.impl.hpp"

namespace saucer
{
    uri::uri() : m_impl(std::make_unique<impl>()) {}

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
        return m_impl->uri.toString().toStdString();
    }

    fs::path uri::path() const
    {
        return m_impl->uri.path().toStdString();
    }

    std::string uri::scheme() const
    {
        return m_impl->uri.scheme().toStdString();
    }

    std::optional<std::string> uri::host() const
    {
        auto rtn = m_impl->uri.host();

        if (rtn.isEmpty())
        {
            return std::nullopt;
        }

        return rtn.toStdString();
    }

    std::optional<std::string> uri::query() const
    {
        if (!m_impl->uri.hasQuery())
        {
            return std::nullopt;
        }

        return m_impl->uri.query().toStdString();
    }

    std::optional<std::size_t> uri::port() const
    {
        auto rtn = m_impl->uri.port();

        if (rtn == -1)
        {
            return std::nullopt;
        }

        return static_cast<std::size_t>(rtn);
    }

    std::optional<std::string> uri::user() const
    {
        auto rtn = m_impl->uri.userName();

        if (rtn.isEmpty())
        {
            return std::nullopt;
        }

        return rtn.toStdString();
    }

    std::optional<std::string> uri::password() const
    {
        auto rtn = m_impl->uri.password();

        if (rtn.isEmpty())
        {
            return std::nullopt;
        }

        return rtn.toStdString();
    }

    std::optional<uri> uri::from(const fs::path &file)
    {
        auto rtn = QUrl::fromLocalFile(QString::fromStdString(file.string()));

        if (!rtn.isValid())
        {
            return std::nullopt;
        }

        return impl{rtn};
    }

    std::optional<uri> uri::parse(const std::string &input)
    {
        auto rtn = QUrl::fromUserInput(QString::fromStdString(input));

        if (!rtn.isValid())
        {
            return std::nullopt;
        }

        return impl{rtn};
    }

    uri uri::make(const options &opts)
    {
        auto rtn = QUrl{};

        rtn.setPath(QString::fromStdString(opts.path.string()));
        rtn.setScheme(QString::fromStdString(opts.scheme));

        if (opts.host.has_value())
        {
            rtn.setHost(QString::fromStdString(opts.host.value()));
        }

        if (opts.query.has_value())
        {
            rtn.setQuery(QString::fromStdString(opts.query.value()));
        }

        if (opts.port.has_value())
        {
            rtn.setPort(static_cast<int>(opts.port.value()));
        }

        return impl{rtn};
    }
} // namespace saucer
