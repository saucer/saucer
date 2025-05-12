#pragma once

#include "scheme.hpp"

#include "webview.hpp"
#include "cocoa.utils.hpp"

#import <WebKit/WebKit.h>
#include <lockpp/lock.hpp>

namespace saucer::scheme
{
    using task_ref = utils::objc_obj<id<WKURLSchemeTask>>;

    struct request::impl
    {
        friend void init_objc();

      public:
        task_ref task;
    };

    struct callback
    {
        application *app;
        scheme::resolver resolver;
    };

    void init_objc();
} // namespace saucer::scheme

@interface SchemeHandler : NSObject <WKURLSchemeHandler>
{
  @public
    std::unordered_map<WKWebView *, saucer::scheme::callback> m_callbacks;
    lockpp::lock<std::unordered_map<NSUInteger, saucer::scheme::task_ref>> m_tasks;
}
- (void)add_callback:(saucer::scheme::callback)callback webview:(WKWebView *)instance;
- (void)del_callback:(WKWebView *)instance;
@end
