#pragma once

#include "runner.hpp"

#include <boost/ut.hpp>
#include <saucer/smartview.hpp>

#include <cstdint>
#include <functional>
#include <string_view>

namespace saucer::tests
{
    inline saucer::application *g_application;

    enum launch : std::uint8_t
    {
        sync  = 1 << 0,
        async = 1 << 1,
    };

    template <typename T>
    struct make;

    template <>
    struct make<saucer::window>
    {
        static auto operator()()
        {
            auto rtn = saucer::window::create(g_application).value();
            rtn->show();
            return rtn;
        }
    };

    template <>
    struct make<saucer::webview>
    {
        static auto operator()()
        {
            return saucer::webview::create({.window = make<saucer::window>{}()}).value();
        }
    };

    template <>
    struct make<saucer::smartview>
    {
        static auto operator()()
        {
            return saucer::smartview::create({.window = make<saucer::window>{}()}).value();
        }
    };

    template <std::uint8_t Policy>
    struct test
    {
        std::string_view name;

      private:
        static std::string_view store(std::string str)
        {
            // boost-ut stores the name as a string_view internally.
            // Unfortunately, this conflicts with our dynamic test names...

            static std::array<std::string, 150> storage;
            static std::size_t i = 0;

            const auto index  = i++;
            storage.at(index) = std::move(str);

            return storage.at(index);
        }

      private:
        template <typename T, typename Callback>
        void invoke(Callback &&callback)
        {
            auto cb = [callback = std::forward<Callback>(callback)]() mutable
            {
                auto instance = make<T>{}();

                if constexpr (std::same_as<T, saucer::window>)
                {
                    return callback(*instance);
                }
                else
                {
                    return callback(instance);
                }
            };

            if constexpr (Policy & sync)
            {
                boost::ut::test(store(std::format("{}:seq", name))) = cb;
            }

            if constexpr (Policy & async)
            {
                boost::ut::test(store(std::format("{}:par", name))) = cb;
            }
        }

      public:
        template <typename T>
            requires std::invocable<T, saucer::window &>
        constexpr void operator=(T &&callback) // NOLINT(*-assign*)
        {
            return invoke<saucer::window>(std::forward<T>(callback));
        }

        template <typename T>
            requires std::invocable<T, saucer::webview &>
        constexpr void operator=(T &&callback) // NOLINT(*-assign*)
        {
            return invoke<saucer::webview>(std::forward<T>(callback));
        }

        template <typename T>
            requires(std::invocable<T, saucer::smartview &> and not std::invocable<T, saucer::webview &>)
        constexpr void operator=(T &&callback) // NOLINT(*-assign*)
        {
            return invoke<saucer::smartview>(std::forward<T>(callback));
        }
    };

    inline auto operator""_test_sync(const char *name, std::size_t size)
    {
        return test<sync>{.name = {name, size}};
    }

    inline auto operator""_test_async(const char *name, std::size_t size)
    {
        return test<async>{.name = {name, size}};
    }

    inline auto operator""_test_both(const char *name, std::size_t size)
    {
        return test<sync | async>{.name = {name, size}};
    }
} // namespace saucer::tests
