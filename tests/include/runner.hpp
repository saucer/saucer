#pragma once

#include <boost/ut.hpp>

#include <coco/task/task.hpp>
#include <coco/latch/latch.hpp>

#include <thread>

namespace saucer::tests
{
    struct co_runner : boost::ut::runner<>
    {
        using boost::ut::runner<>::on;

        coco::task<bool> run()
        {
            filter_ = std::string_view{"*:seq"};

            for (const auto &[suite, _] : suites_)
            {
                suite();
            }

            filter_    = std::string_view{"*:par"};
            auto latch = coco::latch{1};

            std::thread thread{[this, &latch]()
                               {
                                   for (const auto &[suite, _] : suites_)
                                   {
                                       suite();
                                   }

                                   latch.count_down();
                               }};

            thread.detach();
            co_await latch;

            suites_.clear();

            co_return fails_ > 0;
        }
    };
} // namespace saucer::tests

template <>
inline auto boost::ut::cfg<boost::ut::override> = saucer::tests::co_runner{};
