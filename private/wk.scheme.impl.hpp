#pragma once

#include "scheme.hpp"

#import <WebKit/WebKit.h>

namespace saucer
{
    struct request::impl
    {
        friend void init_request_objc();

      public:
        id<WKURLSchemeTask> task;
    };

    void init_request_objc();
} // namespace saucer

@interface SchemeHandler : NSObject <WKURLSchemeHandler>
{
  @public
    std::unordered_map<void *, saucer::scheme_handler> m_handlers;
}
- (void)add_handler:(saucer::scheme_handler)handler webview:(WKWebView *)instance;
- (void)remove_handler:(WKWebView *)instance;
@end
