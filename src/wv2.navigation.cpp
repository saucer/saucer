#include "wv2.navigation.impl.hpp"

#include "win32.utils.hpp"
#include "utils/overload.hpp"

namespace saucer
{
    navigation::navigation(impl data) : m_impl(std::make_unique<impl>(data)) {}

    navigation::navigation(const navigation &other) : navigation(*other.m_impl) {}

    navigation::~navigation() = default;

    uri navigation::url() const
    {
        utils::string_handle raw;

        auto visitor = [&](auto *request)
        {
            request->get_Uri(&raw.reset());
        };
        std::visit(visitor, m_impl->request);

        return uri::parse(utils::narrow(raw.get())).value_or({});
    }

    bool navigation::new_window() const
    {
        auto visitor = overload{[](ICoreWebView2NewWindowRequestedEventArgs *) { return true; },
                                [](ICoreWebView2NavigationStartingEventArgs *)
                                {
                                    return false;
                                }};

        return std::visit(visitor, m_impl->request);
    }

    bool navigation::redirection() const
    {
        auto visitor = overload{[](ICoreWebView2NewWindowRequestedEventArgs *) { return false; },
                                [](ICoreWebView2NavigationStartingEventArgs *request)
                                {
                                    BOOL rtn{};
                                    request->get_IsRedirected(&rtn);

                                    return static_cast<bool>(rtn);
                                }};

        return std::visit(visitor, m_impl->request);
    }

    bool navigation::user_initiated() const
    {
        BOOL rtn{};

        auto visitor = [&](auto *request)
        {
            request->get_IsUserInitiated(&rtn);
        };
        std::visit(visitor, m_impl->request);

        return static_cast<bool>(rtn);
    }
} // namespace saucer
