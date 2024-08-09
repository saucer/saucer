#include "qt.webview.impl.hpp"

#include "qt.window.impl.hpp"

#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

#include <fmt/core.h>

namespace saucer
{
    void webview::inject(const std::string &code, load_time time, web_frame frame)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, code, time, frame] { inject(code, time, frame); }).get();
        }

        QWebEngineScript script;
        bool found = false;

        switch (time)
        {
        case load_time::creation:
            script = m_impl->web_view->page()->scripts().findScript("_creation");
            found  = !script.isNull();

            script.setInjectionPoint(QWebEngineScript::DocumentCreation);
            script.setName("_creation");
            break;

        case load_time::ready:
            script = m_impl->web_view->page()->scripts().findScript("_ready");
            found  = !script.isNull();

            script.setInjectionPoint(QWebEngineScript::DocumentReady);
            script.setName("_ready");
            break;
        }

        if (!found)
        {
            script.setRunsOnSubFrames(false);
            script.setWorldId(QWebEngineScript::MainWorld);
        }
        else
        {
            m_impl->web_view->page()->scripts().remove(script);
        }

        auto source = code;

        if (frame == web_frame::top)
        {
            source = fmt::format(R"js(
            if (self === top)
            {{
                {}
            }}
            )js",
                                 code);
        }

        script.setSourceCode(script.sourceCode() + "\n" + QString::fromStdString(source));
        m_impl->web_view->page()->scripts().insert(script);
    }
} // namespace saucer
