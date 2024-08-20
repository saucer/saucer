#pragma once

#include <chrono>
#include <functional>

#include <print>
#include <source_location>

#include <boost/ut.hpp>
#include <saucer/webview.hpp>

namespace test
{
    namespace chrono = std::chrono;
    using clock      = chrono::system_clock;

    class wait_guard
    {
        using pred_t = std::function<bool()>;

      private:
        pred_t m_pred;

      private:
        std::source_location m_loc;
        std::chrono::seconds m_timeout;

      public:
        wait_guard(pred_t pred, chrono::seconds timeout = chrono::seconds{10},
                   std::source_location loc = std::source_location::current())
            : m_pred(std::move(pred)), m_timeout(timeout), m_loc(loc)
        {
        }

      public:
        ~wait_guard()
        {
            const auto start = chrono::system_clock::now();

            while (true)
            {
                const auto now = chrono::system_clock::now();

                if (now - start > m_timeout)
                {
                    break;
                }

                if (std::invoke(m_pred))
                {
                    return;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            std::println("Timeout reached: {}:{}", m_loc.file_name(), m_loc.line());
        }
    };

    struct load_guard : wait_guard
    {
        load_guard(saucer::webview &webview, bool &value, chrono::seconds timeout = chrono::seconds{10},
                   std::source_location loc = std::source_location::current())
            : wait_guard([&webview, &value]() { return value; }, timeout, loc)
        {
            value = false;
        }
    };

    struct url_guard : wait_guard
    {
        url_guard(saucer::webview &webview, std::string url, chrono::seconds timeout = chrono::seconds{10},
                  std::source_location loc = std::source_location::current())
            : wait_guard([&webview, url = std::move(url)]() { return webview.url().contains(url); }, timeout, loc)
        {
        }
    };

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
