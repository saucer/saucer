#pragma once

#include <print>
#include <thread>

#include <chrono>
#include <source_location>

#include <boost/ut.hpp>

namespace test
{
    using namespace std::chrono_literals;

    template <typename T>
    void wait_for(const T &pred, std::chrono::seconds timeout = 10s,
                  std::source_location source = std::source_location::current())
    {
        using clock      = std::chrono::system_clock;
        const auto start = clock::now();

        auto check = [&pred]()
        {
            if constexpr (std::is_invocable_v<T>)
            {
                return std::invoke(pred);
            }
            else
            {
                return pred;
            }
        };

        while (true)
        {
            const auto now = clock::now();

            if (now - start > timeout)
            {
                std::println("Timeout reached: {}:{}", source.file_name(), source.line());
                break;
            }

            if (!std::invoke(check))
            {
                std::this_thread::sleep_for(500ms);
                continue;
            }

            break;
        }
    }

    struct runner : boost::ut::runner<>
    {
        using boost::ut::runner<>::on;
        using boost::ut::runner<>::run;

        template <class... Ts>
        auto on(boost::ut::events::test<Ts...> test)
        {
            std::println("|- {}", test.name);
            return boost::ut::runner<>::on(test);
        }
    };
} // namespace test

template <class... Ts>
inline auto boost::ut::cfg<boost::ut::override, Ts...> = test::runner{};
