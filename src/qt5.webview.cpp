#include "qt.webview.impl.hpp"

#include "qt.window.impl.hpp"

#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

namespace saucer
{
    void webview::inject(const std::string &java_script, const load_time &load_time)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return dispatch([this, java_script, load_time] { inject(java_script, load_time); }).get();
        }

        QWebEngineScript script;
        bool found = false;

        switch (load_time)
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
            script.setSourceCode(QString::fromStdString(java_script));
        }
        else
        {
            m_impl->web_view->page()->scripts().remove(script);
            script.setSourceCode(script.sourceCode() + "\n" + QString::fromStdString(java_script));
        }

        m_impl->web_view->page()->scripts().insert(script);
    }
} // namespace saucer
