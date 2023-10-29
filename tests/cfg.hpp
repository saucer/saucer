#pragma once

#include <boost/ut.hpp>

namespace cfg
{
    struct runner : public boost::ut::runner<>
    {
        using boost::ut::runner<>::on;
        using boost::ut::runner<>::run;

        template <class... Ts>
        auto on(boost::ut::events::test<Ts...> test)
        {
            std::cout << "Running Test: " << test.name << std::endl;
            boost::ut::runner<>::on(test);
        }
    };
} // namespace cfg

template <class... Ts>
inline auto boost::ut::cfg<boost::ut::override, Ts...> = cfg::runner{};
