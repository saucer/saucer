#include <array>
#include <boost/ut.hpp>
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

    "evaluate"_test = [&]
    {
        auto callback = [&](int result)
        {
            called.at(i++).set_value(true);
            expect(eq(result, 4));
        };

        smartview.evaluate<int>("Math.pow({}, {})", 2, 2) | saucer::then(callback);
        smartview.evaluate<int>("Math.pow({})", saucer::make_args(2, 2)) | saucer::then(callback);
    };

    "expose"_test = [&]
    {
        smartview.expose("f1", [&] { called[2].set_value(true); });

        smartview.expose("f2",
                         [&](int a, const std::string &b)
                         {
                             called[3].set_value(true);

                             expect(eq(a, 10));
                             expect(b == "hello!") << b;
                         });

        smartview.expose(
            "f3",
            [&](custom_type custom)
            {
                called[4].set_value(true);

                for (auto &flag : called)
                {
                    expect(flag.get_future().get());
                }

                expect(eq(custom.field, 1337));

                smartview.close();
            },
            true);

        smartview.evaluate<void>("window.saucer.call({}, [])", "f1") | saucer::forget();

        smartview.evaluate<void>("window.saucer.call({})", saucer::make_args("f2", std::make_tuple(10, "hello!"))) |
            saucer::forget();

        smartview.evaluate<void>("window.saucer.call({}, {})", "f3", std::make_tuple(custom_type{.field = 1337})) |
            saucer::forget();
    };

    smartview.set_url("https://github.com");
    smartview.show();
    smartview.run();
};