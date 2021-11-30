#pragma once
#include <bridge/bridge.hpp>
#include <webview.hpp>

namespace saucer
{
    template <typename bridge_t> class bridged_webview : public webview, public bridge_t
    {
        static_assert(internal::is_bridge_t<bridge_t>);

      protected:
        void on_message(const std::string &) override;

      public:
        bridged_webview();
    };
} // namespace saucer

#include "bridged_webview.inl"