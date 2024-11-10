#include "wk.scheme.impl.hpp"

#import <objc/objc-runtime.h>

void saucer::scheme::init_objc()
{
    static bool init = false;

    if (init)
    {
        return;
    }

    class_replaceMethod(
        [SchemeHandler class], @selector(webView:startURLSchemeTask:),
        imp_implementationWithBlock(
            [](SchemeHandler *self, WKWebView *instance, id<WKURLSchemeTask> task)
            {
                const utils::autorelease_guard guard{};

                if (!self->m_callbacks.contains(instance))
                {
                    return;
                }

                auto handle = [self, task]
                {
                    auto locked = self->m_tasks.write();
                    return locked->emplace(task.hash, task_ref::ref(task)).first->first;
                }();

                auto resolve = [self, handle](const scheme::response &response)
                {
                    auto tasks = self->m_tasks.write();

                    if (!tasks->contains(handle))
                    {
                        return;
                    }

                    auto task          = tasks->at(handle);
                    const auto content = response.data;

                    auto *const data = [NSData dataWithBytes:content.data() length:static_cast<NSInteger>(content.size())];
                    auto *const headers = [[[NSMutableDictionary<NSString *, NSString *> alloc] init] autorelease];

                    for (const auto &[key, value] : response.headers)
                    {
                        [headers setObject:[NSString stringWithUTF8String:value.c_str()]
                                    forKey:[NSString stringWithUTF8String:key.c_str()]];
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
                    auto tasks = self->m_tasks.write();

                    if (!tasks->contains(handle))
                    {
                        return;
                    }

                    auto task = tasks->at(handle);

                    [task.get() didFailWithError:[NSError errorWithDomain:NSURLErrorDomain
                                                                     code:std::to_underlying(error)
                                                                 userInfo:nil]];

                    tasks->erase(handle);
                };

                auto &[app, policy, resolver] = self->m_callbacks.at(instance);

                auto req      = scheme::request{{task}};
                auto executor = scheme::executor{std::move(resolve), std::move(reject)};

                if (policy != launch::async)
                {
                    return std::invoke(resolver, std::move(req), std::move(executor));
                }

                app->pool().emplace([resolver, executor = std::move(executor), req = std::move(req)]() mutable
                                    { std::invoke(resolver, std::move(req), std::move(executor)); });
            }),
        "v@:@");

    init = true;
}

@implementation SchemeHandler
- (void)add_callback:(saucer::scheme::callback)callback webview:(WKWebView *)instance
{
    saucer::scheme::init_objc();
    m_callbacks.emplace(instance, std::move(callback));
}

- (void)del_callback:(WKWebView *)instance
{
    m_callbacks.erase(instance);
}

- (void)webView:(nonnull WKWebView *)webView startURLSchemeTask:(nonnull id<WKURLSchemeTask>)urlSchemeTask
{
}

- (void)webView:(nonnull WKWebView *)webView stopURLSchemeTask:(nonnull id<WKURLSchemeTask>)urlSchemeTask
{
    const saucer::utils::autorelease_guard guard{};

    auto tasks = m_tasks.write();
    tasks->erase(urlSchemeTask.hash);
}
@end
