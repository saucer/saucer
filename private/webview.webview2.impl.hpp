#pragma once
#include "webview.hpp"

#include <optional>
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
        bool is_ready{false};

      public:
        std::vector<LPCWSTR> injected;
        std::vector<std::string> scripts_once;
        std::vector<std::string> scripts_load;

      public:
        std::optional<EventRegistrationToken> event_token;

      public:
        static constexpr std::string_view scheme_prefix = "https://saucer/";
        static constexpr std::wstring_view scheme_prefix_w = L"https://saucer/";

      public:
        static WNDPROC o_wnd_proc;
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
    };
} // namespace saucer