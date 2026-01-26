#include "qt.url.impl.hpp"

namespace saucer
{
    url::url() : m_impl(std::move(make({.scheme = "about", .path = "blank"}).m_impl)) {}

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
        return m_impl->url.toString().toStdString();
    }

    fs::path url::path() const
    {
        return m_impl->url.path().toStdString();
    }

    std::string url::scheme() const
    {
        return m_impl->url.scheme().toStdString();
    }

    std::optional<std::string> url::host() const
    {
        auto rtn = m_impl->url.host();

        if (rtn.isEmpty())
        {
            return std::nullopt;
        }

        return rtn.toStdString();
    }

    std::optional<std::size_t> url::port() const
    {
        auto rtn = m_impl->url.port();

        if (rtn == -1)
        {
            return std::nullopt;
        }

        return static_cast<std::size_t>(rtn);
    }

    std::optional<std::string> url::user() const
    {
        auto rtn = m_impl->url.userName();

        if (rtn.isEmpty())
        {
            return std::nullopt;
        }

        return rtn.toStdString();
    }

    std::optional<std::string> url::password() const
    {
        auto rtn = m_impl->url.password();

        if (rtn.isEmpty())
        {
            return std::nullopt;
        }

        return rtn.toStdString();
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
        auto rtn = QUrl::fromLocalFile(QString::fromStdString(file.string()));

        if (!rtn.isValid())
        {
            return err(error{
                .code     = -1,
                .message  = rtn.errorString().toStdString(),
                .kind     = unknown_domain(),
                .location = std::source_location::current(),
            });
        }

        return impl{rtn};
    }

    result<url> url::parse(cstring_view input)
    {
        if (std::string_view{input}.empty())
        {
            return url{};
        }

        auto rtn = QUrl::fromUserInput(QString::fromUtf8(input));

        if (!rtn.isValid())
        {
            return err(error{
                .code     = -1,
                .message  = rtn.errorString().toStdString(),
                .kind     = unknown_domain(),
                .location = std::source_location::current(),
            });
        }

        return impl{rtn};
    }

    url url::make(const options &opts)
    {
        auto rtn = QUrl{};

        rtn.setScheme(QString::fromStdString(opts.scheme));

        if (opts.host.has_value())
        {
            rtn.setHost(QString::fromStdString(*opts.host));
        }

        if (opts.port.has_value())
        {
            rtn.setPort(static_cast<int>(*opts.port));
        }

        rtn.setPath(QString::fromStdString(opts.path.string()));

        return impl{rtn};
    }
} // namespace saucer
