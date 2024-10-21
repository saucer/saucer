#pragma once

#include <thread>
#include <print>

#include <functional>
#include <chrono>

#include <source_location>

namespace saucer::tests
{
    using namespace std::chrono_literals;

    template <typename T>
    void wait_for(const T &pred, std::chrono::seconds timeout = 5s,
                  std::source_location source = std::source_location::current())
    {
        using clock      = std::chrono::system_clock;
        const auto start = clock::now();

        auto check = [&pred]
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
                std::println("[{}:{}] Timeout reached", source.file_name(), source.line());
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
} // namespace saucer::tests
