#include "qt.webview.impl.hpp"

#include <algorithm>

#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

#include <fmt/core.h>

namespace saucer
{
    void webview::inject(const script &script)
    {
        if (!m_parent->thread_safe())
        {
            return dispatch([this, script] { inject(script); });
        }

        if (script.permanent && !std::ranges::contains(m_impl->permanent_scripts, script))
        {
            m_impl->permanent_scripts.emplace_back(script);
        }

        QWebEngineScript web_script;
        bool found = false;

        switch (script.time)
        {
        case load_time::creation:
            web_script = m_impl->web_view->page()->scripts().findScript("_creation");
            found      = !web_script.isNull();

            web_script.setInjectionPoint(QWebEngineScript::DocumentCreation);
            web_script.setName("_creation");
            break;

        case load_time::ready:
            web_script = m_impl->web_view->page()->scripts().findScript("_ready");
            found      = !web_script.isNull();

            web_script.setInjectionPoint(QWebEngineScript::DocumentReady);
            web_script.setName("_ready");
            break;
        }

        if (!found)
        {
            web_script.setRunsOnSubFrames(false);
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
