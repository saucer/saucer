#include "wk.navigation.impl.hpp"

namespace saucer
{
    navigation::navigation(impl data) : m_impl(std::make_unique<impl>(data)) {}

    navigation::navigation(const navigation &other) : navigation(*other.m_impl) {}

    navigation::~navigation() = default;

    std::string navigation::url() const
    {
        return m_impl->action.request.URL.absoluteString.UTF8String;
    }

    bool navigation::redirection() const
    {
#ifdef SAUCER_WEBKIT_PRIVATE
        // https://github.com/WebKit/WebKit/blob/7240fde26436fed0bf903826c90f596c0207c5ae/Source/WebKit/UIProcess/API/Cocoa/WKNavigationAction.mm#L215
        return reinterpret_cast<NSNumber *>([m_impl->action valueForKey:@"_isRedirect"]).boolValue;
#else
        return false;
#endif
    }

    bool navigation::new_window() const
    {
        return !m_impl->action.targetFrame;
    }

    bool navigation::user_initiated() const
    {
#ifdef SAUCER_WEBKIT_PRIVATE
        // https://github.com/WebKit/WebKit/blob/7240fde26436fed0bf903826c90f596c0207c5ae/Source/WebKit/UIProcess/API/Cocoa/WKNavigationAction.mm#L180C9-L180C25
        return reinterpret_cast<NSNumber *>([m_impl->action valueForKey:@"_isUserInitiated"]).boolValue;
#else
        return false;
#endif
    }
} // namespace saucer
