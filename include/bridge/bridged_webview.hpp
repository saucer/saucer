#pragma once
#include <bridge/bridge.hpp>
#include <module/module.hpp>
#include <set>
#include <webview.hpp>

namespace saucer
{
    template <class bridge_t> class bridged_webview : public webview, public bridge_t
    {
        static_assert(internal::is_bridge_t<bridge_t>);

      private:
        std::set<std::shared_ptr<module>> m_modules;

      protected:
        void on_message(const std::string &) override;

      public:
        bridged_webview();

      public:
        template <template <typename> typename module_t> void use_module();
    };
} // namespace saucer

#include "bridged_webview.inl"