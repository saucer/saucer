#include "webview.hpp"
#include "window.qt.impl.hpp"
#include "webview.qt.impl.hpp"

#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

namespace saucer
{
    void webview::on_message(const std::string &message)
    {
        if (message == "js_ready")
        {
            m_impl->is_ready = true;
            return;
        }

        if (message == "js_finished")
        {
            auto script = m_impl->web_view->page()->scripts().findScript("_run_js");

            if (!script.isNull())
            {
                return;
            }

            m_impl->web_view->page()->scripts().remove(script);
        }
    }

    void webview::run_java_script(const std::string &java_script)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([this, java_script] { run_java_script(java_script); });
        }

        if (m_impl->is_ready)
        {
            m_impl->web_view->page()->runJavaScript(QString::fromStdString(java_script));
            return;
        }

        auto script = m_impl->web_view->page()->scripts().findScript("_run_js");

        if (script.isNull())
        {
            script.setName("_run_js");
            script.setRunsOnSubFrames(false);
            script.setWorldId(QWebEngineScript::MainWorld);
            script.setInjectionPoint(QWebEngineScript::DocumentReady);
            script.setSourceCode(R"(window.saucer.on_message("js_finished");)");
        }
        else
        {
            m_impl->web_view->page()->scripts().remove(script);
        }

        script.setSourceCode(script.sourceCode() + "\n" + QString::fromStdString(java_script));
        m_impl->web_view->page()->scripts().insert(script);
    }

    void webview::inject(const std::string &java_script, const load_time &load_time)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([java_script, load_time, this] { inject(java_script, load_time); });
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
