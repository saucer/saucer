#include "wk.navigation.impl.hpp"

#include "wk.uri.impl.hpp"

namespace saucer
{
    navigation::navigation(impl data) : m_impl(std::make_unique<impl>(std::move(data))) {}

    navigation::~navigation() = default;

    uri navigation::url() const
    {
        return uri::impl{[m_impl->action.get().request.URL copy]};
    }

    bool navigation::redirection() const
    {
#ifdef SAUCER_WEBKIT_PRIVATE
        // https://github.com/WebKit/WebKit/blob/7240fde26436fed0bf903826c90f596c0207c5ae/Source/WebKit/UIProcess/API/Cocoa/WKNavigationAction.mm#L215
        return reinterpret_cast<NSNumber *>([m_impl->action.get() valueForKey:@"_isRedirect"]).boolValue;
#else
        return false;
#endif
    }

    bool navigation::new_window() const
    {
        return !m_impl->action.get().targetFrame;
    }

    bool navigation::user_initiated() const
    {
#ifdef SAUCER_WEBKIT_PRIVATE
        // https://github.com/WebKit/WebKit/blob/7240fde26436fed0bf903826c90f596c0207c5ae/Source/WebKit/UIProcess/API/Cocoa/WKNavigationAction.mm#L180C9-L180C25
        return reinterpret_cast<NSNumber *>([m_impl->action.get() valueForKey:@"_isUserInitiated"]).boolValue;
#else
        return false;
#endif
    }
} // namespace saucer
