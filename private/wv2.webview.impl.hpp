#pragma once

#include "webview.hpp"

#include <string>

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
    using TitleChanged         = ICoreWebView2DocumentTitleChangedEventHandler;
    using NavigationComplete   = ICoreWebView2NavigationCompletedEventHandler;
    using WebMessageHandler    = ICoreWebView2WebMessageReceivedEventHandler;
    using NavigationStarting   = ICoreWebView2NavigationStartingEventHandler;
    using NewWindowRequest     = ICoreWebView2NewWindowRequestedEventHandler;
    using DOMLoaded            = ICoreWebView2DOMContentLoadedEventHandler;
    using FaviconChanged       = ICoreWebView2FaviconChangedEventHandler;
    using GetFavicon           = ICoreWebView2GetFaviconCompletedHandler;
    using SourceChanged        = ICoreWebView2SourceChangedEventHandler;

    struct webview::impl
    {
        ComPtr<ICoreWebView2Controller> controller;
        ComPtr<ICoreWebView2Settings> settings;
        ComPtr<ICoreWebView2> web_view;

      public:
        icon favicon;

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;

      public:
        std::vector<std::pair<script, std::wstring>> scripts;
        std::unordered_map<std::string, scheme_handler> schemes;

      public:
        WNDPROC o_wnd_proc;

      public:
        void set_wnd_proc(HWND hwnd);
        void create_webview(webview *, HWND, saucer::options);

      public:
        HRESULT scheme_handler(ICoreWebView2WebResourceRequestedEventArgs *);

      public:
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);

      public:
        template <web_event>
        void setup(webview *);

      public:
        static const std::string &inject_script();

      public:
        static inline ULONG_PTR gdi_token;
        static inline auto env_options = Make<CoreWebView2EnvironmentOptions>();
    };
} // namespace saucer
