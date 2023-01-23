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
            auto script = m_impl->web_view->page()->scripts().find("_run_js");

            if (script.empty())
            {
                return;
            }

            m_impl->web_view->page()->scripts().remove(script.first());
        }
    }

    void webview::run_java_script(const std::string &java_script)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { run_java_script(java_script); });
        }

        if (m_impl->is_ready)
        {
            m_impl->web_view->page()->runJavaScript(QString::fromStdString(java_script));
            return;
        }

        QWebEngineScript script;
        auto scripts = m_impl->web_view->page()->scripts().find("_run_js");

        if (!scripts.empty())
        {
            script = scripts.first();
            m_impl->web_view->page()->scripts().remove(script);
        }
        else
        {
            script.setName("_run_js");
            script.setRunsOnSubFrames(false);
            script.setWorldId(QWebEngineScript::MainWorld);
            script.setInjectionPoint(QWebEngineScript::DocumentReady);
            script.setSourceCode(R"(window.saucer.on_message("js_finished");)");
        }

        script.setSourceCode(QString::fromStdString(java_script) + "\n" + script.sourceCode());
        m_impl->web_view->page()->scripts().insert(script);
    }

    void webview::inject(const std::string &java_script, const load_time &load_time)
    {
        if (!window::m_impl->is_thread_safe())
        {
            return window::m_impl->post_safe([=] { inject(java_script, load_time); });
        }

        QWebEngineScript script;
        bool found = false;

        auto check_previous = [&](const char *name) {
            auto scripts = m_impl->web_view->page()->scripts().find(name);

            if (scripts.empty())
            {
                return;
            }

            found = true;
            script = scripts.front();
        };

        switch (load_time)
        {
        case load_time::creation:
            check_previous("_creation");
            script.setName("_creation");
            script.setInjectionPoint(QWebEngineScript::DocumentCreation);
            break;

        case load_time::ready:
            check_previous("_ready");
            script.setName("_ready");
            script.setInjectionPoint(QWebEngineScript::DocumentCreation);
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