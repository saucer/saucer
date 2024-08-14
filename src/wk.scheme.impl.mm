#include "wk.scheme.impl.hpp"

#import <objc/objc-runtime.h>

void saucer::init_request_objc()
{
    static bool init = false;

    if (init)
    {
        return;
    }

    class_replaceMethod([SchemeHandler class], @selector(webView:startURLSchemeTask:),
                        imp_implementationWithBlock(
                            [](SchemeHandler *self, WKWebView *instance, id<WKURLSchemeTask> urlSchemeTask)
                            {
                                auto *id = (__bridge void *)instance;
                                auto req = saucer::request{{urlSchemeTask}};

                                if (!self->m_handlers.contains(id))
                                {
                                    return;
                                }

                                auto result = std::invoke(self->m_handlers[id], req);

                                if (!result.has_value())
                                {
                                    // TODO
                                    return;
                                }

                                auto content        = result->data;
                                auto *const data    = [NSData dataWithBytes:content.data() length:content.size()];
                                auto *const headers = [[NSMutableDictionary<NSString *, NSString *> alloc] init];

                                for (const auto &[key, value] : result->headers)
                                {
                                    [headers setObject:[NSString stringWithUTF8String:value.c_str()]
                                                forKey:[NSString stringWithUTF8String:key.c_str()]];
                                }

                                [headers setObject:@"*" forKey:@"Access-Control-Allow-Origin"];
                                [headers setObject:[NSString stringWithUTF8String:result->mime.c_str()]
                                            forKey:@"Content-Type"];
                                [headers setObject:[NSString stringWithFormat:@"%zu", content.size()]
                                            forKey:@"Content-Length"];

                                auto *const response = [[NSHTTPURLResponse alloc] initWithURL:urlSchemeTask.request.URL
                                                                                   statusCode:200
                                                                                  HTTPVersion:nil
                                                                                 headerFields:headers];

                                [urlSchemeTask didReceiveResponse:response];
                                [urlSchemeTask didReceiveData:data];
                                [urlSchemeTask didFinish];
                            }),
                        "v@:@");

    init = true;
}

@implementation SchemeHandler
- (void)add_handler:(saucer::scheme_handler)handler webview:(WKWebView *)instance
{
    saucer::init_request_objc();

    auto *id = (__bridge void *)instance;

    if (m_handlers.contains(id))
    {
        return;
    }

    m_handlers.emplace(id, std::move(handler));
}

- (void)remove_handler:(WKWebView *)instance
{
    auto *id = (__bridge void *)instance;
    m_handlers.erase(id);
}

- (void)webView:(nonnull WKWebView *)webView startURLSchemeTask:(nonnull id<WKURLSchemeTask>)urlSchemeTask
{
}

- (void)webView:(nonnull WKWebView *)webView stopURLSchemeTask:(nonnull id<WKURLSchemeTask>)urlSchemeTask
{
}
@end
