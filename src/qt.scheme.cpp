#include "qt.scheme.impl.hpp"

#include "error.impl.hpp"
#include "qt.url.impl.hpp"

#include <ranges>

#include <QMap>
#include <QByteArray>

namespace saucer::scheme
{
    struct writer
    {
        std::shared_ptr<stash_stream::native> platform;

      public:
        bool operator()(stash::span);
    };

    bool writer::operator()(stash::span data)
    {
        platform->device->append(data);
        return true;
    }

    result<stream> response::stream()
    {
        auto rtn         = stash(std::make_unique<stash_stream>());
        auto *const impl = static_cast<stash_stream *>(rtn.native<false>());

        return scheme::stream{
            .stash = std::move(rtn),
            .write = writer{impl->platform},
        };
    }

    request::request(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    request::request(const request &other) : request(*other.m_impl) {}

    request::request(request &&) noexcept = default;

    request::~request() = default;

    url request::url() const
    {
        const auto request = m_impl->request->write();
        return url::impl{request.value()->requestUrl()};
    }

    std::string request::method() const
    {
        const auto request = m_impl->request->write();
        return request.value()->requestMethod().toStdString();
    }

    stash request::content() const
    {
        const auto *data = reinterpret_cast<const std::uint8_t *>(m_impl->body.data());
        return stash::view({data, data + m_impl->body.size()});
    }

    std::map<std::string, std::string> request::headers() const
    {
        const auto request = m_impl->request->write();
        const auto headers = request.value()->requestHeaders();

        auto transform = [&headers](auto &item)
        {
            return std::make_pair(item.toStdString(), headers[item].toStdString());
        };

        return headers                            //
               | std::views::transform(transform) //
               | std::ranges::to<std::map<std::string, std::string>>();
    }
} // namespace saucer::scheme
