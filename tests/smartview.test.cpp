#include "cfg.hpp"

#include <array>
#include <thread>
#include <saucer/smartview.hpp>
#include <saucer/utils/future.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

struct custom_type
{
    int field;
};

template <>
struct glz::meta<custom_type>
{
    using T                     = custom_type;
    static constexpr auto value = object("field", &T::field);
};

suite smartview_suite = []
{
    saucer::smartview smartview({.hardware_acceleration = false});

    std::size_t i{0};
    std::array<std::promise<bool>, 6> called{};
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

        smartview.expose(
            "f4",
            [&](const std::string &utf8)
            {
                std::cout << "f4 called" << std::endl;
                called[5].set_value(true);

                expect(utf8 == "測試-тест");
                expect(smartview.evaluate<std::string>("'測試-тест'").get() == "測試-тест");
            },
            true);

        std::async(std::launch::deferred,
                   [&]
                   {
                       expect(called[0].get_future().get());
                       expect(called[1].get_future().get());

                       saucer::all(
                           smartview.evaluate<void>("window.saucer.call({}, [])", "f1"),
                           smartview.evaluate<void>("window.saucer.call({})",
                                                    saucer::make_args("f2", std::make_tuple(10, "hello!"))),
                           smartview.evaluate<void>("window.saucer.call({}, {})", "f3",
                                                    std::make_tuple(custom_type{.field = 1337})),
                           smartview.evaluate<void>("window.saucer.call({}, {})", "f4", std::make_tuple("測試-тест")));

                       expect(called[2].get_future().get());
                       expect(called[3].get_future().get());
                       expect(called[4].get_future().get());
                       expect(called[5].get_future().get());

                       smartview.close();
                   }) |
            saucer::forget();
    };

    smartview.set_url("https://www.google.com");
    smartview.show();
    smartview.run();
};
