#pragma once

#include "webview.hpp"
#include "scheme.qt.impl.hpp"

#include <string>
#include <vector>

#include <QMetaObject>
#include <QWebChannel>
#include <QWebEngineView>
#include <QWebEngineUrlSchemeHandler>

namespace saucer
{
    //! The web_view should be the first member of the impl struct, as it should
    //! be easily accessible for modules.

    struct webview::impl
    {
        class web_class;

      public:
        QWebEngineView *web_view;

      public:
        QWebEnginePage *page;

      public:
        QObject *channel_obj;
        QWebChannel *web_channel;

      public:
        QWebEngineView *dev_view;

      public:
        bool dom_loaded{false};
        std::vector<std::string> pending;
        std::map<std::string, url_scheme_handler> schemes;

      public:
        QMetaObject::Connection url_changed;
        QMetaObject::Connection load_finished;

      public:
        template <web_event>
        void setup(webview *);

      public:
        static const std::string ready_script;
        static const std::string inject_script;
    };

    class webview::impl::web_class : public QObject
    {
        Q_OBJECT

      private:
        webview *m_parent;

      public:
        web_class(webview *);

      public slots:
        void on_message(const QString &);
    };
} // namespace saucer
