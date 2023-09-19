#include "cfg.hpp"

#include <array>
#include <saucer/smartview.hpp>
#include <saucer/utils/future.hpp>
#include <thread>

using namespace boost::ut;
using namespace boost::ut::literals;

struct custom_type
{
    int field;
};

template <>
struct glz::meta<custom_type>
{
    using T = custom_type;
    static constexpr auto value = object( //
        "field", &T::field                //
    );
};

// NOLINTNEXTLINE
suite smartview_suite = []
{
    saucer::smartview smartview;

    std::size_t i{0};
    std::array<std::promise<bool>, 5> called{};
    auto thread_id = std::this_thread::get_id();

    "evaluate"_test = [&]
    {
        auto callback = [&](int result)
        {
            std::cout << "evaluate called (" << i << ")" << std::endl;

            called.at(i++).set_value(true);
            expect(eq(result, 4));
        };

        smartview.evaluate<int>("Math.pow({}, {})", 2, 2) | saucer::then(callback);
        smartview.evaluate<int>("Math.pow({})", saucer::make_args(2, 2)) | saucer::then(callback);
    };

    "expose"_test = [&]
    {
        smartview.expose("f1",
                         [&]
                         {
                             std::cout << "f1 called" << std::endl;
                             called[2].set_value(true);
                         });

        smartview.expose("f2",
                         [&](int a, const std::string &b)
                         {
                             std::cout << "f2 called" << std::endl;
                             called[3].set_value(true);

                             expect(eq(a, 10));
                             expect(b == "hello!") << b;
                         });

        smartview.expose(
            "f3",
            [&](custom_type custom)
            {
                std::cout << "f3 called" << std::endl;
                called[4].set_value(true);

                expect(eq(custom.field, 1337));
                expect(neq(std::this_thread::get_id(), thread_id));
            },
            true);

        std::async(std::launch::deferred,
                   [&]
                   {
                       expect(called[0].get_future().get());
                       expect(called[1].get_future().get());

                       saucer::all(smartview.evaluate<void>("window.saucer.call({}, [])", "f1"),
                                   smartview.evaluate<void>("window.saucer.call({})",
                                                            saucer::make_args("f2", std::make_tuple(10, "hello!"))),
                                   smartview.evaluate<void>("window.saucer.call({}, {})", "f3",
                                                            std::make_tuple(custom_type{.field = 1337})));

                       expect(called[2].get_future().get());
                       expect(called[3].get_future().get());
                       expect(called[4].get_future().get());

                       smartview.close();
                   }) |
            saucer::forget();
    };

    smartview.set_url("https://github.com");
    smartview.show();
    smartview.run();
};