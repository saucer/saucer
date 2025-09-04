#pragma once

#include <random>
#include <limits>

#include <chrono>
#include <thread>

#include <functional>
#include <algorithm>

namespace saucer::tests
{
    inline auto &random_engine()
    {
        static auto device = std::random_device{};
        static auto seed   = std::seed_seq{device(), device(), device(), device()};
        static auto engine = std::mt19937{seed};

        return engine;
    }

    template <typename T = int>
    auto random(T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max())
    {
        static auto dist = std::uniform_int_distribution<T>{min, max};
        return dist(random_engine());
    }

    inline auto random_string(std::size_t length = random())
    {
        auto generate = [](auto...) -> char
        {
            return static_cast<char>('A' + random(0, 25));
        };

        auto rtn = std::string(length, 'A');
        std::ranges::generate(rtn, generate);

        return rtn;
    }

    template <typename Callable>
        requires std::invocable<Callable>
    auto wait_for(Callable callable, std::chrono::seconds duration = std::chrono::seconds(1))
    {
        using clock = std::chrono::system_clock;

        auto status = false;
        auto limit  = clock::now() + duration;

        while (not(status = std::invoke(callable)) && clock::now() < limit)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        return status;
    }
} // namespace saucer::tests
