#include "wv2.navigation.impl.hpp"

#include "win32.utils.hpp"
#include "utils/overload.hpp"

namespace saucer
{
    navigation::navigation(impl data) : m_impl(std::make_unique<impl>(data)) {}

    navigation::navigation(const navigation &other) : navigation(*other.m_impl) {}

    navigation::~navigation() = default;

    std::string navigation::url() const
    {
        auto visitor = [](auto *request)
        {
            utils::string_handle raw;
            request->get_Uri(&raw.reset());
            return utils::narrow(raw.get());
        };

        return std::visit(visitor, m_impl->request);
    }

    bool navigation::new_window() const
    {
        overload visitor = {
            [](ICoreWebView2NewWindowRequestedEventArgs *) { return true; },
            [](ICoreWebView2NavigationStartingEventArgs *) { return false; },
        };

        return std::visit(visitor, m_impl->request);
    }

    bool navigation::redirection() const
    {
        overload visitor = {
            [](ICoreWebView2NewWindowRequestedEventArgs *) { return false; },
            [](ICoreWebView2NavigationStartingEventArgs *request)
            {
                BOOL rtn{};
                request->get_IsRedirected(&rtn);

                return static_cast<bool>(rtn);
            },
        };

        return std::visit(visitor, m_impl->request);
    }

    bool navigation::user_initiated() const
    {
        auto visitor = [](auto *request)
        {
            BOOL rtn{};
            request->get_IsUserInitiated(&rtn);

            return static_cast<bool>(rtn);
        };

        return std::visit(visitor, m_impl->request);
    }
} // namespace saucer
