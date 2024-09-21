#pragma once

#include "utils/required.hpp"

#include <string>
#include <memory>

#include <functional>

namespace saucer
{
    struct options
    {
        required<std::string> id;
    };

    struct application
    {
        struct impl;

      private:
        using callback_t = std::move_only_function<void()>;

      private:
        std::unique_ptr<impl> m_impl;

      private:
        void dispatch(callback_t) const;

      private:
        application(const options &);

      public:
        ~application();

      public:
        [[nodiscard]] impl *native() const;
        [[nodiscard]] bool thread_safe() const;

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
