#include "qt.webview.impl.hpp"

#include "qt.window.impl.hpp"

#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

#include <algorithm>
#include <fmt/core.h>

namespace saucer
{
    void webview::inject(const script &script)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, script] { inject(script); }).get();
        }

        if (script.permanent && !std::ranges::contains(m_impl->permanent_scripts, script))
        {
            m_impl->permanent_scripts.emplace_back(script);
        }

        QWebEngineScript web_script;
        bool found = false;

        auto check_previous = [&](const char *name)
        {
            auto scripts = m_impl->web_view->page()->scripts().find(name);

            if (scripts.empty())
            {
                return;
            }

            found      = true;
            web_script = scripts.front();
        };

        switch (script.time)
        {
        case load_time::creation:
            check_previous("_creation");
            web_script.setName("_creation");
            web_script.setInjectionPoint(QWebEngineScript::DocumentCreation);
            break;

        case load_time::ready:
            check_previous("_ready");
            web_script.setName("_ready");
            web_script.setInjectionPoint(QWebEngineScript::DocumentReady);
            break;
        }

        if (!found)
        {
            web_script.setRunsOnSubFrames(true);
            web_script.setWorldId(QWebEngineScript::MainWorld);
        }
        else
        {
            m_impl->web_view->page()->scripts().remove(web_script);
        }

        auto source = script.code;

        if (script.frame == web_frame::top)
        {
            source = fmt::format(R"js(
            if (self === top)
            {{
                {}
            }}
            )js",
                                 source);
        }

        web_script.setSourceCode(web_script.sourceCode() + "\n" + QString::fromStdString(source));
        m_impl->web_view->page()->scripts().insert(web_script);
    }
} // namespace saucer
