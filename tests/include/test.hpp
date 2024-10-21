#pragma once

#include <functional>

#include <boost/ut.hpp>
#include <saucer/smartview.hpp>

namespace saucer::tests
{
    namespace impl
    {
        template <typename T>
        struct safe_deleter
        {
            std::shared_ptr<saucer::application> app;

          public:
            void operator()(T *ptr)
            {
                if (!app->thread_safe())
                {
                    return app->dispatch([this, ptr] { return (*this)(ptr); }).get();
                }

                delete ptr;
            };
        };

        template <typename T>
        std::shared_ptr<T> get(std::shared_ptr<saucer::application> app)
        {
            if (!app->thread_safe())
            {
                return app->dispatch([app] { return get<T>(app); }).get();
            }

            auto *ptr = new T{{.application = app}};
            ptr->show();

            return {ptr, impl::safe_deleter<T>{app}};
        }

        enum launch : int
        {
            async = 1 << 0,
            sync  = 1 << 1,
        };

        template <int Policy, typename T = saucer::smartview<>>
        struct test
        {
            std::string name;

          public:
            constexpr auto operator=(std::function<void(std::shared_ptr<T>)> test) // NOLINT(*-assign*)
            {
                return boost::ut::test(name) = [test = std::move(test)]
                {
                    auto app = saucer::application::acquire({""});

                    if constexpr (Policy & launch::sync)
                    {
                        std::invoke(test, get<T>(app));
                    }

                    if constexpr (Policy & launch::async)
                    {
                        auto fut = std::async(std::launch::async, test, get<T>(app));
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
