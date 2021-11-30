#pragma once
#include <bridge/bridged_webview.hpp>

namespace saucer
{
    template <typename bridge_t> bridged_webview<bridge_t>::bridged_webview() : bridge_t(dynamic_cast<webview &>(*this)) {}
    template <typename bridge_t> void bridged_webview<bridge_t>::on_message(const std::string &message)
    {
        webview::on_message(message);
        bridge_t::on_message(message);
    }
} // namespace saucer