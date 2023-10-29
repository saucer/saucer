#pragma once

#include "webview.hpp"

#include <any>
#include <optional>
#include <concepts>
#include <string_view>

#include <wrl.h>
#include <WebView2.h>

namespace saucer
{
    //! The webview and controller should be the first members of the impl struct, as they should
    //! be easily accessible for modules.

    using Microsoft::WRL::Callback;
    using Microsoft::WRL::ComPtr;
    using Microsoft::WRL::Make;

    struct webview::impl
    {
        ComPtr<ICoreWebView2> web_view;
        ComPtr<ICoreWebView2Controller> controller;

      public:
        WNDPROC original_wnd_proc;

      public:
        std::vector<LPCWSTR> injected;

      public:
        std::vector<std::string> pending;
        std::vector<std::string> scripts;

      public:
        EventRegistrationToken url_changed{};
        EventRegistrationToken load_finished{};
        EventRegistrationToken scheme_handler{};

      public:
        bool dom_loaded{false};

      public:
        static const std::string inject_script;
        static constexpr std::string_view scheme_prefix = "saucer://embedded/";

      public:
        void overwrite_wnd_proc(HWND hwnd);
        void install_scheme_handler(webview *);
        void create_webview(webview *, HWND, saucer::options);

      public:
        static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);

      public:
        template <web_event>
        void setup(webview *);
    };

    /*
     * Is this kind of dumb? Yes.
     * Are these interface names also dumb? "ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler" - Yes.
     * => So I'll use this "Magic Callback" class to keep me sane while writing webview2 code...
     */
    template <typename Func>
    class mcb
    {
        Func m_func;
        std::any m_value;

      public:
        mcb(Func func) : m_func(std::move(func)) {}

      public:
        template <typename T>
        operator T *()
        {
            using callback_t = decltype(Callback<T>(nullptr));

            if (!m_value.has_value())
            {
                m_value = Callback<T>(m_func);
            }

            return std::any_cast<callback_t>(m_value).Get();
        }
    };
} // namespace saucer
