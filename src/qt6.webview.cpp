#include "qt.webview.impl.hpp"

#include <QWebEngineScriptCollection>

namespace saucer
{
    using native = webview::impl::native;

    QWebEngineScript native::find(const char *name) const
    {
        return web_page->scripts().find(name).at(0);
    }
} // namespace saucer
