#pragma once

#include "webview.hpp"

#include <string>
#include <optional>

#include <wrl.h>
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>

namespace saucer
{
    using Microsoft::WRL::Callback;
    using Microsoft::WRL::ComPtr;
    using Microsoft::WRL::Make;

    // These type-names are straight from hell. Thanks microsoft!
    using ScriptInjected       = ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler;
    using EnvironmentCompleted = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;
    using ControllerCompleted  = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
    using ResourceRequested    = ICoreWebView2WebResourceRequestedEventHandler;
    using NavigationComplete   = ICoreWebView2NavigationCompletedEventHandler;
    using WebMessageHandler    = ICoreWebView2WebMessageReceivedEventHandler;
    using NavigationStarting   = ICoreWebView2NavigationStartingEventHandler;
    using NewWindowRequest     = ICoreWebView2NewWindowRequestedEventHandler;
    using DOMLoaded            = ICoreWebView2DOMContentLoadedEventHandler;
    using SourceChanged        = ICoreWebView2SourceChangedEventHandler;

    struct webview::impl
    {
        ComPtr<ICoreWebView2Controller> controller;
        ComPtr<ICoreWebView2Settings> settings;
        ComPtr<ICoreWebView2> web_view;

      public:
        WNDPROC original_wnd_proc;

      public:
        std::vector<LPCWSTR> injected;
        std::vector<std::string> scripts;

      public:
        std::optional<EventRegistrationToken> load_token;
        std::optional<EventRegistrationToken> scheme_token;
        std::optional<EventRegistrationToken> navigation_token;

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;
        std::map<std::string, scheme_handler> schemes;

      public:
        static const std::string &inject_script();

      public:
        void overwrite_wnd_proc(HWND hwnd);
        void create_webview(webview *, HWND, saucer::options);
        HRESULT scheme_handler(ICoreWebView2WebResourceRequestedEventArgs *);

      public:
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);

      public:
        template <web_event>
        void setup(webview *);

      public:
        static inline auto env_options = Make<CoreWebView2EnvironmentOptions>();
    };
} // namespace saucer
