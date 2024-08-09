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

        auto check_previous = [&](const char *name)
        {
            auto scripts = m_impl->web_view->page()->scripts().find(name);

            if (scripts.empty())
            {
                return;
            }

            found  = true;
            script = scripts.front();
        };

        switch (time)
        {
        case load_time::creation:
            check_previous("_creation");
            script.setName("_creation");
            script.setInjectionPoint(QWebEngineScript::DocumentCreation);
            break;

        case load_time::ready:
            check_previous("_ready");
            script.setName("_ready");
            script.setInjectionPoint(QWebEngineScript::DocumentReady);
            break;
        }

        if (!found)
        {
            script.setRunsOnSubFrames(true);
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
