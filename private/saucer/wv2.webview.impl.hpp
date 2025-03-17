#pragma once

#include "webview.hpp"

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
        ComPtr<ICoreWebView2_3> web_view;

      public:
        icon favicon;

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;

      public:
        std::uint32_t browser_pid;
        std::optional<fs::path> temp_path;

      public:
        std::vector<std::pair<script, std::wstring>> scripts;
        std::unordered_map<std::string, std::pair<scheme::resolver, launch>> schemes;

      public:
        void create_webview(const std::shared_ptr<application> &, HWND, preferences);
        HRESULT scheme_handler(ICoreWebView2WebResourceRequestedEventArgs *, webview *);

      public:
        template <web_event>
        void setup(webview *);

      public:
        static std::string inject_script();
        static ComPtr<CoreWebView2EnvironmentOptions> env_options();

      public:
        WNDPROC o_wnd_proc;
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };
} // namespace saucer
