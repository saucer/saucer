#include "cfg.hpp"

#include <format>

#include <saucer/smartview.hpp>
#include <saucer/utils/future.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

struct data
{
    int x;
};

using saucer::forget;

static void tests(saucer::smartview<> &webview)
{
    "sync_methods"_test = [&]()
    {
        expect(webview.evaluate<int>("await saucer.exposed.a()").get() == 10);
        expect(webview.evaluate<int>("await saucer.exposed.b(10)").get() == 20);
        expect(webview.evaluate<std::string>("await saucer.exposed.c('Hello C++', 23)").get() == "Hello C++23");
    };

    "complex_param"_test = [&]()
    {
        expect(webview.evaluate<int>("await saucer.exposed.d({{x: 15}})").get() == 30);
    };

    const auto *code = R"js(
        await (async () => {{
            try
            {{
                return (await saucer.call({}, [{}])).toString();
            }}
            catch (err)
            {{
                return err;
            }}
        }})()
    )js";

    "executor"_test = [&]()
    {
        expect(webview.evaluate<std::string>(code, "e", -100).get() == "expected positive number");
        expect(webview.evaluate<std::string>(code, "e", 100).get() == "100");

        expect(webview.evaluate<std::string>(code, "f", -100).get() == "expected positive number");
        expect(webview.evaluate<std::string>(code, "f", 100).get() == "105");
    };

    "async_methods"_test = [&]()
    {
        expect(webview.evaluate<int>("await saucer.exposed.g()").get() == 15);
        forget(webview.evaluate<void>("await saucer.exposed.h()"));
    };
}

suite<"smartview"> smartview_suite = []
{
    saucer::smartview smartview{{.hardware_acceleration = false}};

    smartview.expose("close", [&]() { smartview.close(); });

    smartview.expose("a", []() { return 10; });
    smartview.expose("b", [](int param) { return param + 10; });
    smartview.expose("c", [](std::string text, int param) { return std::format("{}{}", text, param); });

    smartview.expose("d", [](const data &param) { return param.x * 2; });

    smartview.expose("e",
                     [](int param, const saucer::executor<int> &exec)
                     {
                         const auto &[resolve, reject] = exec;

                         if (param < 0)
                         {
                             return reject("expected positive number");
                         }

                         resolve(param);
                     });
    smartview.expose(
        "f",
        [&](int param, const saucer::executor<int> &exec)
        {
            const auto &[resolve, reject] = exec;

            if (param < 0)
            {
                return reject("expected positive number");
            }

            resolve(smartview.evaluate<int>("{} + 5", param).get());
        },
        saucer::launch::async);

    smartview.expose("g", [&]() { return smartview.evaluate<int>("10 + 5").get(); }, saucer::launch::async);
    smartview.expose("h", [&]() { forget(smartview.evaluate<void>("saucer.exposed.close()")); }, saucer::launch::async);

    const std::jthread thread{[&]()
                              {
                                  std::this_thread::sleep_for(std::chrono::seconds(2));
                                  tests(smartview);
                              }};

    smartview.set_url("https://saucer.github.io/");
    smartview.show();

    smartview.run();
};
