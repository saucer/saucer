#pragma once

#include <saucer/scheme.hpp>

#include "cocoa.utils.hpp"

#import <WebKit/WebKit.h>
#include <lockpp/lock.hpp>

namespace saucer::scheme
{
    using task_ref = utils::objc_obj<id<WKURLSchemeTask>>;

    struct request::impl
    {
        task_ref task;
    };
} // namespace saucer::scheme

@interface SchemeHandler : NSObject <WKURLSchemeHandler>
{
  @public
    std::unordered_map<WKWebView *, saucer::scheme::resolver> m_callbacks;
    lockpp::lock<std::unordered_map<NSUInteger, saucer::scheme::task_ref>> m_tasks;
}
- (void)add_callback:(saucer::scheme::resolver)callback webview:(WKWebView *)instance;
- (void)del_callback:(WKWebView *)instance;
@end
