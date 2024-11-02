#pragma once

#include "utils/required.hpp"
#include "modules/module.hpp"

#include <string>
#include <memory>

#include <functional>

namespace saucer
{
    struct options
    {
        required<std::string> id;
    };

    struct application : extensible<application>
    {
        struct impl;

      private:
        using callback_t = std::move_only_function<void()>;

      private:
        std::unique_ptr<impl> m_impl;

      private:
        application(const options &);

      public:
        ~application();

      public:
        template <bool Stable = true>
        [[nodiscard]] natives<application, Stable> native() const;

      public:
        [[nodiscard]] bool thread_safe() const;

      public:
        void post(callback_t) const;

      public:
        template <typename Callback>
        [[sc::thread_safe]] auto dispatch(Callback &&) const;

      public:
        template <bool Blocking = true>
        [[sc::may_block]] void run() const;

      public:
        void quit();

      public:
        [[nodiscard]] static std::shared_ptr<application> acquire(const options &);
    };
} // namespace saucer

#include "app.inl"
