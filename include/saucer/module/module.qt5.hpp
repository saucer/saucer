#pragma once
#include "module.hpp"
#include <QMainWindow>
#include <QWebEngineView>

namespace saucer
{
    template <> struct module<backend_type::qt>::webview_impl
    {
      private:
        [[maybe_unused]] char _pad[sizeof(void *) + sizeof(std::unique_ptr<void>)];

      public:
        QWebEngineView *web_view;
    };

    template <> struct module<backend_type::qt>::window_impl
    {
        QMainWindow *window;
    };
} // namespace saucer