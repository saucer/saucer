#pragma once

#include "modules/module.hpp"
#include "modules/traits/app.hpp"

#include "utils/required.hpp"

#include <vector>
#include <utility>
#include <optional>
#include <functional>

#include <string>
#include <memory>

#include <coco/stray/stray.hpp>
#include <coco/promise/promise.hpp>

namespace saucer
{
    struct options
    {
        required<std::string> id;

      public:
        std::optional<int> argc;
        std::optional<char **> argv;

      public:
        bool quit_on_last_window_closed{true};
    };

    struct screen
    {
        std::string name;

      public:
        std::pair<int, int> size;
        std::pair<int, int> position;
    };

    template <typename T>
    struct safe_delete;

    template <typename T>
    using safe_ptr = std::unique_ptr<T, safe_delete<T>>;

    struct application : modules::extend<application>
    {
        struct impl;

      private:
        using post_callback_t = std::move_only_function<void()>;
        using callback_t      = std::move_only_function<coco::stray(application *)>;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        application(impl);

      public:
        ~application();

      public:
        template <bool Stable = true>
        [[nodiscard]] natives<application, Stable> native() const;

      public:
        [[nodiscard]] bool thread_safe() const;
        [[nodiscard]] coco::future<void> finish();
        [[nodiscard]] std::vector<screen> screens() const;

      public:
        void post(post_callback_t) const;

      public:
        template <bool Get = true, typename Callback>
        [[sc::thread_safe]] auto dispatch(Callback &&) const;

        template <typename T, typename... Ts>
        [[sc::thread_safe]] safe_ptr<T> make(Ts &&...) const;

      public:
        void quit();
        int run(callback_t);

      public:
        void operator&() = delete;

      public:
        [[nodiscard]] static std::optional<application> create(const options &);
    };
} // namespace saucer

#include "app.inl"
