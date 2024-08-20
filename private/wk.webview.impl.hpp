#pragma once

#include "webview.hpp"

#include "wk.scheme.impl.hpp"
#include "cocoa.window.impl.hpp"

#include <vector>
#include <unordered_map>

#import <WebKit/WebKit.h>

@class MessageHandler;
@class NavigationDelegate;

namespace saucer
{
    struct webview::impl
    {
        WKWebView *web_view;

      public:
        NSAppearance *appearance;
        NavigationDelegate *delegate;

      public:
        WKWebViewConfiguration *config;
        WKUserContentController *controller;

      public:
        bool force_dark{false};
        bool context_menu{false};

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;

      public:
        template <web_event>
        void setup(webview *);

      public:
        static void init_objc();
        static WKWebViewConfiguration *make_config(const options &);

      public:
        static const std::string &inject_script();
        static constinit std::string_view ready_script;

      public:
        static inline std::unordered_map<std::string, SchemeHandler *> schemes;
    };
} // namespace saucer

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
