#include "wk.scheme.impl.hpp"

using namespace saucer;
using namespace saucer::scheme;

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

    auto ref = task_ref::ref(task);

    auto handle = [&]
    {
        auto locked = self->m_tasks.write();
        return locked->emplace(task.hash, ref).first->first;
    }();

    auto resolve = [self, handle](const scheme::response &response)
    {
        const utils::autorelease_guard guard{};

        auto tasks = self->m_tasks.write();

        if (!tasks->contains(handle))
        {
            return;
        }

        auto task          = tasks->at(handle);
        const auto content = response.data;

        auto *const data    = [NSData dataWithBytes:content.data() length:static_cast<NSInteger>(content.size())];
        auto *const headers = [[[NSMutableDictionary<NSString *, NSString *> alloc] init] autorelease];

        for (const auto &[key, value] : response.headers)
        {
            [headers setObject:[NSString stringWithUTF8String:value.c_str()] forKey:[NSString stringWithUTF8String:key.c_str()]];
        }

        auto *const mime   = [NSString stringWithUTF8String:response.mime.c_str()];
        auto *const length = [NSString stringWithFormat:@"%zu", content.size()];

        [headers setObject:mime forKey:@"Content-Type"];
        [headers setObject:length forKey:@"Content-Length"];

        auto *const res = [[[NSHTTPURLResponse alloc] initWithURL:task.get().request.URL
                                                       statusCode:response.status
                                                      HTTPVersion:nil
                                                     headerFields:headers] autorelease];

        [task.get() didReceiveResponse:res];
        [task.get() didReceiveData:data];
        [task.get() didFinish];

        tasks->erase(handle);
    };

    auto reject = [self, handle](const scheme::error &error)
    {
        const utils::autorelease_guard guard{};

        auto tasks = self->m_tasks.write();

        if (!tasks->contains(handle))
        {
            return;
        }

        auto task = tasks->at(handle);

        [task.get() didFailWithError:[NSError errorWithDomain:NSURLErrorDomain code:std::to_underlying(error) userInfo:nil]];

        tasks->erase(handle);
    };

    auto req      = scheme::request{{ref}};
    auto executor = scheme::executor{std::move(resolve), std::move(reject)};

    return std::invoke(self->m_callbacks[instance], std::move(req), std::move(executor));
}

- (void)webView:(nonnull WKWebView *)webview stopURLSchemeTask:(nonnull id<WKURLSchemeTask>)task
{
    const saucer::utils::autorelease_guard guard{};

    auto tasks = m_tasks.write();
    tasks->erase(task.hash);
}
@end
