#pragma once

#include "webview.hpp"

#include "cocoa.utils.hpp"
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
        utils::objc_ptr<WKWebViewConfiguration> config;
        utils::objc_ptr<WKWebView> web_view;

      public:
        utils::objc_ptr<NavigationDelegate> delegate;
        WKUserContentController *controller;

      public:
        NSAppearance *appearance;

      public:
        bool force_dark{false};
        bool context_menu{true};

      public:
        std::vector<script> permanent_scripts;

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;

      public:
        template <web_event>
        void setup(webview *);

      public:
        static void init_objc();
        static WKWebViewConfiguration *make_config(const preferences &);

      public:
        static const std::string &inject_script();
        static constinit std::string_view ready_script;

      public:
        static inline std::unordered_map<std::string, utils::objc_ptr<SchemeHandler>> schemes;
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
