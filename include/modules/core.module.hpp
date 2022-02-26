#pragma once
#include <modules/module.hpp>

namespace saucer
{
    class core_module final : public module
    {
      public:
        ~core_module() final;
        core_module(smartview &);

      public:
        void on_message(const std::string &) override;
        void on_url_changed(const std::string &) override;
    };
} // namespace saucer