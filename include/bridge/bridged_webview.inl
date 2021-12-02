#pragma once
#include <bridge/bridged_webview.hpp>

namespace saucer
{
    template <class bridge_t> bridged_webview<bridge_t>::bridged_webview() : bridge_t(dynamic_cast<webview &>(*this)) {}
    template <class bridge_t> void bridged_webview<bridge_t>::on_message(const std::string &message)
    {
        webview::on_message(message);
        bridge_t::on_message(message);
    }

    template <class bridge_t> template <template <typename> typename module_t> void bridged_webview<bridge_t>::use_module()
    {
        m_modules.emplace(std::make_shared<module_t<bridge_t>>(*this));
    }
} // namespace saucer