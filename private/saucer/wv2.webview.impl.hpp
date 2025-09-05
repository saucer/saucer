#pragma once

#include "webview.impl.hpp"

#include "lease.hpp"

#include <limits>

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
    using Fullscreen           = ICoreWebView2ContainsFullScreenElementChangedEventHandler;
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
        std::size_t id_counter{0};
        std::unordered_map<std::size_t, wv2_script> scripts;
        std::unordered_map<std::string, scheme::resolver> schemes;

      public:
        std::size_t on_resize, on_minimize;
        std::optional<saucer::bounds> bounds;

      public:
        std::uint32_t browser_pid;
        std::optional<fs::path> cleanup;

      public:
        utils::lease<webview::impl *> lease;

      public:
        template <event>
        void setup(impl *);

      public:
        static ComPtr<ICoreWebView2EnvironmentOptions> env_options();

      public:
        static result<fs::path> default_user_folder(std::wstring &);
        static result<ComPtr<ICoreWebView2Environment>> create_environment(application *, const environment_options &);
        static result<ComPtr<ICoreWebView2Controller>> create_controller(application *, HWND, ICoreWebView2Environment *);

      public:
        static HRESULT on_message(impl *, ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *);
        static HRESULT on_resource(impl *, ICoreWebView2 *, ICoreWebView2WebResourceRequestedEventArgs *);

      public:
        static HRESULT on_dom(impl *, ICoreWebView2 *, ICoreWebView2DOMContentLoadedEventArgs *);
        static HRESULT on_navigation(impl *, ICoreWebView2 *, ICoreWebView2NavigationStartingEventArgs *);

      public:
        static HRESULT on_favicon(impl *, ICoreWebView2 *, IUnknown *);
        static HRESULT on_fullscreen(impl *, ICoreWebView2 *, IUnknown *);
        static HRESULT on_window(impl *, ICoreWebView2 *, ICoreWebView2NewWindowRequestedEventArgs *);

      public:
        static HRESULT scheme_handler(impl *, const scheme_options &);

      public:
        static inline auto bound_events = std::numeric_limits<std::size_t>::max();
    };
} // namespace saucer
