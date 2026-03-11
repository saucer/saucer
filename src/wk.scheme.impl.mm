#include "wk.scheme.impl.hpp"

using namespace saucer;
using namespace saucer::scheme;

deferred_task::deferred_task(task_ref task) : task(std::move(task)) {}

deferred_task::~deferred_task()
{
    if (!task)
    {
        return;
    }

    [task.get() didFinish];
}

stash_stream::stash_stream() : platform(std::make_shared<native>()) {}

std::size_t stash_stream::type() const
{
    return id_of<stash_stream>();
}

std::unique_ptr<stash::impl> stash_stream::clone() const
{
    return std::make_unique<stash_stream>(*this);
}

stash::span stash_stream::data() const
{
    return {};
}

void stash_stream::native::attach(deferred_task &&deferred)
{
    auto locked = data.write();

    if (const auto data = std::move(std::get<0>(*locked)); !data.empty())
    {
        [deferred.task.get() didReceiveData:[NSData dataWithBytes:data.data() length:data.size()]];
    }

    locked->emplace<1>(std::move(deferred));
}

@implementation SchemeHandler
- (void)add_callback:(saucer::scheme::resolver)callback webview:(WKWebView *)instance
{
    m_callbacks.emplace(instance, std::move(callback));
}

- (void)del_callback:(WKWebView *)instance
{
    m_callbacks.erase(instance);
}

- (void)webView:(nonnull WKWebView *)instance startURLSchemeTask:(nonnull id<WKURLSchemeTask>)task
{
    const utils::autorelease_guard guard{};

    if (!self->m_callbacks.contains(instance))
    {
        return;
    }

    auto ref          = task_ref::ref(task);
    const auto handle = [&]
    {
        auto locked = self->m_tasks.write();
        return locked->emplace(task.hash, ref).first->first;
    }();

    auto resolve = [self, handle](const scheme::response &response)
    {
        const utils::autorelease_guard guard{};

        auto task = [&]
        {
            auto locked = self->m_tasks.write();
            locked->extract(handle)
        }();

        if (!task)
        {
            return;
        }

        auto *const headers = [[[NSMutableDictionary<NSString *, NSString *> alloc] init] autorelease];
        [headers setObject:[NSString stringWithUTF8String:response.mime.c_str()] forKey:@"Content-Type"];

        for (const auto &[key, value] : response.headers)
        {
            [headers setObject:[NSString stringWithUTF8String:value.c_str()] forKey:[NSString stringWithUTF8String:key.c_str()]];
        }

        auto deferred = deferred_task{std::move(task.mapped())};

        const auto stash  = response.data;
        const auto stream = stash.native<false>()->type() == stash::impl::id_of<stash_stream>();

        const auto data = stash.data();
        const auto size = static_cast<NSInteger>(data.size());

        if (stream)
        {
            [headers setObject:@"chunked" forKey:@"Transfer-Encoding"];
        }
        else
        {
            [headers setObject:[NSString stringWithFormat:@"%zu", size] forKey:@"Content-Length"];
        }

        [deferred.task.get() didReceiveResponse:[[[NSHTTPURLResponse alloc] initWithURL:deferred.task.get().request.URL
                                                                             statusCode:response.status
                                                                            HTTPVersion:@"HTTP/1.1"
                                                                           headerFields:headers] autorelease]];

        if (stream)
        {
            return static_cast<stash_stream *>(stash.native<false>())->platform->attach(std::move(deferred));
        }

        [deferred.task.get() didReceiveData:[NSData dataWithBytes:data.data() length:size]];
    };

    auto reject = [self, handle](const scheme::error &error)
    {
        const utils::autorelease_guard guard{};

        auto tasks = self->m_tasks.write();
        auto task  = tasks->find(handle);

        if (task == tasks->end())
        {
            return;
        }

        [task->second.get() didFailWithError:[NSError errorWithDomain:NSURLErrorDomain //
                                                                 code:std::to_underlying(error)
                                                             userInfo:nil]];

        tasks->erase(task);
    };

    auto req      = scheme::request{{std::move(ref)}};
    auto executor = scheme::executor{std::move(resolve), std::move(reject)};

    return self->m_callbacks[instance](std::move(req), std::move(executor));
}

- (void)webView:(nonnull WKWebView *)webview stopURLSchemeTask:(nonnull id<WKURLSchemeTask>)task
{
    const saucer::utils::autorelease_guard guard{};

    auto tasks = m_tasks.write();
    tasks->erase(task.hash);
}
@end
