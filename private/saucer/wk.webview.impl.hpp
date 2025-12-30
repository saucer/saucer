#pragma once

#include "webview.impl.hpp"

#include "cocoa.utils.hpp"
#include "wk.scheme.impl.hpp"
#include "cocoa.window.impl.hpp"

#include <map>
#include <vector>
#include <unordered_map>

#import <WebKit/WebKit.h>

@class MessageHandler;
@class UIDelegate;
@class NavigationDelegate;

namespace saucer
{
    struct webview::impl::native
    {
        utils::objc_ptr<WKWebViewConfiguration> config;
        utils::objc_ptr<WKWebView> web_view;

      public:
        utils::objc_ptr<UIDelegate> ui_delegate;
        utils::objc_ptr<NavigationDelegate> navigation_delegate;

      public:
        NSAppearance *appearance;
        WKUserContentController *controller;

      public:
        bool force_dark{false};
        bool context_menu{true};

      public:
        std::size_t id_counter{0};
        std::map<std::size_t, script> scripts;

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;

      public:
        std::size_t on_closed;

      public:
        template <event>
        void setup(impl *);

      public:
        void inject(const script &) const;

      public:
        static WKWebViewConfiguration *make_config(const options &);
        static NSUUID *data_store_id(const options &, const std::string &);

      public:
        static inline std::unordered_map<std::string, utils::objc_ptr<SchemeHandler>> schemes;
    };
} // namespace saucer

@interface MessageHandler : NSObject <WKScriptMessageHandler>
{
  @public
    saucer::webview::impl *me;
}
- (instancetype)initWithParent:(saucer::webview::impl *)parent;
@end

@interface UIDelegate : NSObject <WKUIDelegate>
{
  @public
    saucer::webview::impl *me;
}
- (instancetype)initWithParent:(saucer::webview::impl *)parent;
@end

@interface NavigationDelegate : NSObject <WKNavigationDelegate>
{
  @public
    saucer::webview::impl *me;
}
- (instancetype)initWithParent:(saucer::webview::impl *)parent;
@end

@interface SaucerView : WKWebView
{
  @public
    saucer::webview::impl *me;
}
- (instancetype)initWithParent:(saucer::webview::impl *)parent configuration:(WKWebViewConfiguration *)configuration frame:(CGRect)frame;
@end
