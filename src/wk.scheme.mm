#include "wk.scheme.impl.hpp"

#include "wk.url.impl.hpp"
#include "utils/overload.hpp"

namespace saucer::scheme
{
    struct writer
    {
        std::shared_ptr<stash_stream::native> platform;

      public:
        bool operator()(stash::span) const;
    };

    bool writer::operator()(stash::span data) const
    {
        auto locked = platform->data.write();

        std::visit(
            overload{
                [&](stash::vec &vec) { vec.insert_range(vec.end(), data); },
                [&](deferred_task &def) { [def.task.get() didReceiveData:[NSData dataWithBytes:data.data() length:data.size()]]; },
            },
            *locked);

        return true;
    }

    result<stream> response::stream()
    {
        auto rtn         = stash{std::make_unique<stash_stream>()};
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
        return stash::view({raw, raw + body.length});
    }

    std::map<std::string, std::string> request::headers() const
    {
        auto *const headers = m_impl->task.get().request.allHTTPHeaderFields;
        auto rtn            = std::map<std::string, std::string>{};

        [headers enumerateKeysAndObjectsUsingBlock:[&rtn](NSString *key, NSString *value, BOOL *)
                 {
                     rtn.emplace(key.UTF8String, value.UTF8String);
                 }];

        return rtn;
    }
} // namespace saucer::scheme
