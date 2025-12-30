#include "test.hpp"
#include "utils.hpp"

using namespace boost::ut;
using namespace saucer::tests;

suite<"regression"> regression_suite = []
{
    static constexpr auto duration = std::chrono::seconds(10);

    "script-order"_test_async = [](saucer::smartview &webview)
    {
        using enum saucer::script::time;

        // Ensure that the script order is preserved properly.
        // There was a bug where the script order was all messed up after uninjecting a script on MacOS.

        webview.inject({
            .code      = "window.saucer.exposed.log(1)",
            .run_at    = creation,
            .clearable = false,
        });

        webview.inject({
            .code      = "window.saucer.exposed.log(2)",
            .run_at    = creation,
            .clearable = false,
        });

        webview.inject({
            .code      = "window.saucer.exposed.log(3)",
            .run_at    = ready,
            .clearable = false,
        });

        webview.inject({
            .code      = "window.saucer.exposed.log(4)",
            .run_at    = ready,
            .clearable = false,
        });

        auto order = std::vector<int>{};
        webview.expose("log", [&order](int id) { order.emplace_back(id); });

        webview.uninject();
        webview.set_dev_tools(true);
        webview.set_url("https://codeberg.org/saucer/saucer");

        saucer::tests::wait_for([&order] { return order.size() == 4; }, duration);

        expect(eq(order.at(0), 1));
        expect(eq(order.at(1), 2));
        expect(eq(order.at(2), 3));
        expect(eq(order.at(3), 4));
    };
};
