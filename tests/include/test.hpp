#pragma once

#include <functional>

#include <boost/ut.hpp>
#include <saucer/smartview.hpp>

namespace saucer::tests
{
    namespace impl
    {
        enum launch : int
        {
            async = 1 << 0,
            sync  = 1 << 1,
        };

        template <int Policy, typename T = saucer::smartview<>>
        struct test
        {
            std::string name;

          private:
            std::shared_ptr<T> get()
            {
                auto app = application::active();
                auto rtn = app->make<T>(preferences{.application = app});

                rtn->show();

                return rtn;
            }

          public:
            constexpr auto operator=(std::function<void(std::shared_ptr<T>)> test) // NOLINT(*-assign*)
            {
                return boost::ut::test(name) = [this, test = std::move(test)]
                {
                    auto app = application::active();

                    if constexpr (Policy & launch::sync)
                    {
                        std::invoke(test, get());
                    }

                    if constexpr (Policy & launch::async)
                    {
                        auto fut = std::async(std::launch::async, test, get());
                        app->run();
                        fut.get();
                    }
                };
            }
        };
    } // namespace impl

    inline auto operator""_test_sync(const char *name, std::size_t size)
    {
        return impl::test<impl::launch::sync>{.name = {name, size}};
    }

    inline auto operator""_test_async(const char *name, std::size_t size)
    {
        return impl::test<impl::launch::async>{.name = {name, size}};
    }

    inline auto operator""_test_both(const char *name, std::size_t size)
    {
        return impl::test<impl::launch::sync | impl::launch::async>{.name = {name, size}};
    }
} // namespace saucer::tests
