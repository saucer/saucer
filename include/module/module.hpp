#pragma once
#include <string>

namespace saucer
{
    class webview;
    class module
    {
        friend class webview;

      protected:
        webview &m_webview;

      public:
        module(webview &);
        virtual ~module();

      protected:
        virtual void on_url_changed(const std::string &);
    };
} // namespace saucer