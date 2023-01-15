#pragma once
#include "webview.hpp"

#include <wil/com.h>
#include <WebView2.h>
#include <string_view>

namespace saucer
{
    struct webview::impl
    {
        wil::com_ptr<ICoreWebView2> webview;
        wil::com_ptr<ICoreWebView2Controller> controller;

      public:
        std::vector<LPCWSTR> injected;
        std::vector<std::string> scripts_once;
        std::vector<std::string> scripts_load;

      public:
        WNDPROC o_wnd_proc;
        std::unique_ptr<EventRegistrationToken> event_token;

      public:
        bool is_ready{false};

      public:
        static const std::string inject_script;
        static constexpr std::string_view scheme_prefix = "https://saucer/";
        static constexpr std::wstring_view scheme_prefix_w = L"https://saucer/";

      public:
        void create_webview(HWND hwnd, const std::wstring &user_folder);

      public:
        void overwrite_wnd_proc(HWND hwnd);
        void install_scheme_handler(class webview &);

      public:
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };
} // namespace saucer