#pragma once

#include "scheme.hpp"

#import <WebKit/WebKit.h>

namespace saucer::scheme
{
    struct request::impl
    {
        friend void init_objc();

      public:
        id<WKURLSchemeTask> task;
    };

    void init_objc();
} // namespace saucer::scheme

@interface SchemeHandler : NSObject <WKURLSchemeHandler>
{
  @public
    std::unordered_map<void *, saucer::scheme::handler> m_handlers;
}
- (void)add_handler:(saucer::scheme::handler)handler webview:(WKWebView *)instance;
- (void)remove_handler:(WKWebView *)instance;
@end
