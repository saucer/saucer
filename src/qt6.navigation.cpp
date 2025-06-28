#include "qt.navigation.impl.hpp"

#include "utils/overload.hpp"

namespace saucer
{
    navigation::navigation(impl data) : m_impl(std::make_unique<impl>(data)) {}

    navigation::navigation(const navigation &other) : navigation(*other.m_impl) {}

    navigation::~navigation() = default;

    std::string navigation::url() const
    {
        overload visitor = {
            [](QWebEngineNewWindowRequest *request) { return request->requestedUrl().toString().toStdString(); },
            [](QWebEngineNavigationRequest *request) { return request->url().toString().toStdString(); },
        };

        return std::visit(visitor, m_impl->request);
    }

    bool navigation::new_window() const
    {
        overload visitor = {
            [](QWebEngineNewWindowRequest *) { return true; },
            [](QWebEngineNavigationRequest *) { return false; },
        };

        return std::visit(visitor, m_impl->request);
    }

    bool navigation::redirection() const
    {
        overload visitor = {
            [](QWebEngineNewWindowRequest *) { return false; },
            [](QWebEngineNavigationRequest *request)
            { return request->navigationType() == QWebEngineNavigationRequest::RedirectNavigation; },
        };

        return std::visit(visitor, m_impl->request);
    }

    bool navigation::user_initiated() const
    {
        overload visitor = {
            [](QWebEngineNewWindowRequest *request) { return request->isUserInitiated(); },
            [](QWebEngineNavigationRequest *) { return true; },
        };

        return std::visit(visitor, m_impl->request);
    }
} // namespace saucer
