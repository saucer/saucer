#pragma once
#include <bridge/bridged_webview.hpp>
#include <module/module.hpp>

namespace saucer
{
    namespace modules
    {
        template <class bridge_t> class app : public module
        {
          public:
            ~app() override;
            app(bridged_webview<bridge_t> &);
        };
    } // namespace modules
} // namespace saucer

#include "app_module.inl"