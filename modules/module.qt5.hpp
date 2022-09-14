#pragma once
#include <QMainWindow>
#include <QWebEngineView>
#include <saucer/module/module.hpp>

namespace saucer
{
    template <> struct module::webview_impl<backend_type::qt>
    {
      private:
        [[maybe_unused]] char _pad[sizeof(void *) + sizeof(std::unique_ptr<void>)];

      public:
        QWebEngineView *web_view;
    };

    template <> struct module::window_impl<backend_type::qt>
    {
        QMainWindow *window;
    };
} // namespace saucer