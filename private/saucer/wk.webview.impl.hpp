#pragma once

#include "webview.hpp"

#include "cocoa.utils.hpp"
#include "wk.scheme.impl.hpp"
#include "cocoa.window.impl.hpp"

#include <vector>
#include <unordered_map>

#import <WebKit/WebKit.h>

@class MessageHandler;
@class UIDelegate;
@class NavigationDelegate;

namespace saucer
{
    struct webview::impl
    {
        utils::objc_ptr<WKWebViewConfiguration> config;
        utils::objc_ptr<WKWebView> web_view;

      public:
        utils::objc_ptr<NSView> view;

      public:
        utils::objc_ptr<UIDelegate> ui_delegate;
        utils::objc_ptr<NavigationDelegate> navigation_delegate;

      public:
        WKUserContentController *controller;
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
        static WKWebViewConfiguration *make_config(const options &);

      public:
        static std::string inject_script();
        static constinit std::string_view ready_script;

      public:
        static inline std::unordered_map<std::string, utils::objc_ptr<SchemeHandler>> schemes;
    };

    using on_message_t = bool (saucer::webview::*)(std::string_view);
} // namespace saucer

@interface MessageHandler : NSObject <WKScriptMessageHandler>
{
  @public
    saucer::webview *m_parent;
    saucer::webview::events *m_events;
    saucer::on_message_t m_on_message;
}
- (instancetype)initWithParent:(saucer::webview *)parent
                        events:(saucer::webview::events *)events
                     onMessage:(saucer::on_message_t)on_message;
@end

@interface UIDelegate : NSObject <WKUIDelegate>
{
  @public
    saucer::webview *m_parent;
    saucer::webview::events *m_events;
}
- (instancetype)initWithParent:(saucer::webview *)parent events:(saucer::webview::events *)events;
@end

@interface NavigationDelegate : NSObject <WKNavigationDelegate>
{
  @public
    saucer::webview *m_parent;
    saucer::webview::events *m_events;
}
- (instancetype)initWithParent:(saucer::webview *)parent events:(saucer::webview::events *)events;
@end

@interface SaucerView : WKWebView
{
  @public
    saucer::webview *m_parent;
}
- (instancetype)initWithParent:(saucer::webview *)parent configuration:(WKWebViewConfiguration *)configuration frame:(CGRect)frame;
@end
