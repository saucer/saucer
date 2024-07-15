#include "cfg.hpp"

#include <array>

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
    using saucer::launch;

    saucer::smartview smartview({.hardware_acceleration = false});

    std::size_t i{0};
    std::array<std::promise<bool>, 7> called{};

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

        smartview.expose("f3",
                         [&](custom_type custom)
                         {
                             std::cout << "f3 called" << std::endl;

                             called[4].set_value(true);
                             expect(eq(custom.field, 1337));
                         });

        smartview.expose(
            "f4",
            [&](const std::string &utf8)
            {
                std::cout << "f4 called" << std::endl;

                expect(utf8 == "測試-тест");
                expect(smartview.evaluate<std::string>("'測試-тест'").get() == "測試-тест");

                called[5].set_value(true);

                return custom_type{.field = 1337};
            },
            launch::async);

        smartview.expose("f5",
                         [&](int param, const saucer::executor<int> &exec)
                         {
                             const auto &[resolve, reject] = exec;

                             std::cout << "f5 called" << std::endl;

                             called[6].set_value(true);
                             expect(eq(param, 100));

                             resolve(500);
                         });

        std::async(std::launch::deferred,
                   [&]
                   {
                       expect(called[0].get_future().get());
                       expect(called[1].get_future().get());

                       auto all = saucer::all(
                           smartview.evaluate<void>("window.saucer.call({}, [])", "f1"),
                           smartview.evaluate<void>("window.saucer.call({})",
                                                    saucer::make_args("f2", std::make_tuple(10, "hello!"))),
                           smartview.evaluate<void>("window.saucer.call({}, {})", "f3",
                                                    std::make_tuple(custom_type{.field = 1337})),
                           smartview.evaluate<custom_type>("await window.saucer.exposed.f4({})", "測試-тест"),
                           smartview.evaluate<int>("await window.saucer.exposed.f5({})", 100));

                       expect(called[2].get_future().get());
                       expect(called[3].get_future().get());
                       expect(called[4].get_future().get());
                       expect(called[5].get_future().get());
                       expect(called[6].get_future().get());

                       expect(std::get<0>(all).field == 1337);
                       expect(std::get<1>(all) == 500);

                       smartview.close();
                   }) |
            saucer::forget();
    };

    smartview.set_url("https://saucer.github.io");
    smartview.show();
    smartview.run();
};
