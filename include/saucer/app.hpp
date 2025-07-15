#pragma once

#include "modules/module.hpp"
#include "utils/required.hpp"

#include <vector>
#include <optional>
#include <functional>

#include <string>
#include <memory>

#include <coco/stray/stray.hpp>
#include <coco/promise/promise.hpp>

#include <ereignis/manager/manager.hpp>

namespace saucer
{
    enum class policy : std::uint8_t
    {
        allow,
        block,
    };

    struct screen
    {
        std::string name;

      public:
        std::pair<int, int> size;
        std::pair<int, int> position;
    };

    struct application
    {
        struct impl;

      public:
        struct options;

      private:
        using post_callback_t = std::move_only_function<void()>;
        using callback_t      = std::move_only_function<coco::stray(application *)>;

      public:
        enum class event : std::uint8_t
        {
            quit,
        };

      public:
        using events = ereignis::manager<          //
            ereignis::event<event::quit, policy()> //
            >;

      private:
        std::unique_ptr<events> m_events;
        std::unique_ptr<impl> m_impl;

      private:
        application();

      public:
        application(application &&) noexcept;

      public:
        [[nodiscard]] static std::optional<application> create(const options &);

      public:
        ~application();

      public:
        template <bool Stable = true>
        [[nodiscard]] natives<application, Stable> native() const;

      public:
        [[nodiscard]] bool thread_safe() const;
        [[nodiscard]] std::vector<screen> screens() const;

      public:
        void post(post_callback_t) const;

      public:
        int run(callback_t);
        coco::future<void> finish();

      public:
        void quit();

      public:
        void operator&() = delete;

      public:
        template <typename Callback, typename... Ts>
        [[sc::thread_safe]] auto invoke(Callback &&, Ts &&...) const;

      public:
        template <typename T, typename... Ts>
        [[sc::thread_safe]] auto make(Ts &&...) const;
    };

    struct application::options
    {
        required<std::string> id;

      public:
        std::optional<int> argc;
        std::optional<char **> argv;

      public:
        bool quit_on_last_window_closed{true};
    };
} // namespace saucer

#include "app.inl"
