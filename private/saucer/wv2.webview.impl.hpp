#pragma once

#include "webview.impl.hpp"
#include "win32.utils.hpp"

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
    using PermissionRequested  = ICoreWebView2PermissionRequestedEventHandler;
    using WebMessageHandler    = ICoreWebView2WebMessageReceivedEventHandler;
    using NavigationStarting   = ICoreWebView2NavigationStartingEventHandler;
    using NewWindowRequest     = ICoreWebView2NewWindowRequestedEventHandler;
    using DOMLoaded            = ICoreWebView2DOMContentLoadedEventHandler;
    using FaviconChanged       = ICoreWebView2FaviconChangedEventHandler;
    using GetFavicon           = ICoreWebView2GetFaviconCompletedHandler;
    using SourceChanged        = ICoreWebView2SourceChangedEventHandler;

    struct environment_options
    {
        std::wstring storage_path;
        ICoreWebView2EnvironmentOptions *opts;
    };

    struct scheme_options
    {
        ICoreWebView2WebResourceRequestedEventArgs *raw;
        ComPtr<ICoreWebView2WebResourceRequest> request;

      public:
        uri url;
    };

    struct wv2_script : script
    {
        std::optional<std::wstring> ref;
    };

    struct webview::impl::native
    {
        ComPtr<ICoreWebView2Controller> controller;
        ComPtr<ICoreWebView2Settings> settings;
        ComPtr<ICoreWebView2_22> web_view;

      public:
        icon favicon;

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;

      public:
        std::uint64_t id_counter{0};
        std::unordered_map<std::uint64_t, wv2_script> scripts;
        std::unordered_map<std::string, scheme::resolver> schemes;

      public:
        utils::wnd_proc_hook hook;

      public:
        template <event>
        void setup(impl *);

      public:
        static ComPtr<ICoreWebView2EnvironmentOptions> env_options();

      public:
        static result<ComPtr<ICoreWebView2Environment>> create_environment(application *, const environment_options &);
        static result<ComPtr<ICoreWebView2Controller>> create_controller(application *, HWND, ICoreWebView2Environment *);

      public:
        static HRESULT on_message(impl *, ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *);
        static HRESULT on_resource(impl *, ICoreWebView2 *, ICoreWebView2WebResourceRequestedEventArgs *);

      public:
        static HRESULT on_dom(impl *, ICoreWebView2 *, ICoreWebView2DOMContentLoadedEventArgs *);
        static HRESULT on_window(impl *, ICoreWebView2 *, ICoreWebView2NewWindowRequestedEventArgs *);
        static HRESULT on_navigation(impl *, ICoreWebView2 *, ICoreWebView2NavigationStartingEventArgs *);

      public:
        static HRESULT on_favicon(impl *, ICoreWebView2 *, IUnknown *);

      public:
        static HRESULT scheme_handler(impl *, const scheme_options &);
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };
} // namespace saucer
