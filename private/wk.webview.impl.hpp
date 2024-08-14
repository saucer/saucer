#pragma once

#include "webview.hpp"
#include "wk.scheme.impl.hpp"

#include <functional>

#import <WebKit/WebKit.h>

@class MessageHandler;

namespace saucer
{
    using observer_callback_t = std::function<void()>;

    struct webview::impl
    {
        WKWebView *web_view;

      public:
        NSAppearance *default_appearance;
        WKUserContentController *controller;

      public:
        bool dom_loaded{false};
        bool context_menu{false};
        std::vector<std::string> pending;

      public:
        static WKWebViewConfiguration *config();
        static inline std::unordered_map<std::string, SchemeHandler *> schemes;

      public:
        template <web_event>
        void setup(webview *);

      public:
        static const std::string &inject_script();
        static constinit std::string_view ready_script;

      public:
        static void init_objc();
    };
} // namespace saucer

@interface Observer : NSObject
{
  @public
    saucer::observer_callback_t m_callback;
}
- (instancetype)initWithCallback:(saucer::observer_callback_t)callback;
@end

@interface MessageHandler : NSObject <WKScriptMessageHandler>
{
  @public
    saucer::webview *m_parent;
}
- (instancetype)initWithParent:(saucer::webview *)parent;
@end

@interface NavigationDelegate : NSObject <WKNavigationDelegate>
{
  @public
    saucer::webview *m_parent;
}
- (instancetype)initWithParent:(saucer::webview *)parent;
@end

@interface SaucerView : WKWebView
{
  @public
    saucer::webview *m_parent;
}
- (instancetype)initWithParent:(saucer::webview *)parent
                 configuration:(WKWebViewConfiguration *)configuration
                         frame:(CGRect)frame;
@end
