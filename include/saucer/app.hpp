#pragma once

#include "utils/required.hpp"
#include "modules/module.hpp"

#include <vector>
#include <utility>
#include <optional>
#include <functional>

#include <string>
#include <memory>
#include <thread>

namespace saucer
{
    template <typename T>
    struct safe_deleter;

    struct screen
    {
        std::string name;

      public:
        std::pair<int, int> size;
        std::pair<int, int> position;
    };

    struct options
    {
        required<std::string> id;

      public:
        std::optional<int> argc;
        std::optional<char **> argv;

      public:
        std::size_t threads = std::thread::hardware_concurrency();
    };

    struct application : extensible<application>
    {
        struct impl;

      private:
        template <typename T>
        using safe_ptr   = std::unique_ptr<T, safe_deleter<T>>;
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
        [[nodiscard]] std::vector<screen> screens() const;

      public:
        void post(callback_t) const;

      public:
        template <bool Get = true, typename Callback>
        [[sc::thread_safe]] auto dispatch(Callback &&) const;

      public:
        template <typename T, typename... Ts>
        [[sc::thread_safe]] safe_ptr<T> make(Ts &&...) const;

      public:
        template <bool Blocking = true>
        [[sc::may_block]] void run() const;

      public:
        void quit();

      public:
        [[nodiscard]] static std::shared_ptr<application> init(const options &);
        [[nodiscard]] static std::shared_ptr<application> active();
    };
} // namespace saucer

#include "app.inl"
