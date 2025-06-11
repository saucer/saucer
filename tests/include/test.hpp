#pragma once

#include "runner.hpp"

#include <boost/ut.hpp>
#include <saucer/smartview.hpp>

#include <cstdint>
#include <functional>

namespace saucer::tests
{
    inline saucer::application *g_application;

    enum launch : std::uint8_t
    {
        sync  = 1 << 0,
        async = 1 << 1,
    };

    template <std::uint8_t Policy, typename T = saucer::smartview<>>
    struct test
    {
        std::string name;

      private:
        auto make()
        {
            auto rtn = g_application->make<T>(saucer::preferences{.application = g_application});
            rtn->show();

            return rtn;
        }

      public:
        constexpr void operator=(std::function<void(T &)> test) // NOLINT(*-assign*)
        {
            auto callback = [this, test = std::move(test)]() mutable
            {
                std::invoke(test, *make());
            };

            if constexpr (Policy & sync)
            {
                boost::ut::test(std::format("{}:seq", name)) = callback;
            }

            if constexpr (Policy & async)
            {
                boost::ut::test(std::format("{}:par", name)) = callback;
            }
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
