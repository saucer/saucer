#include "test.hpp"

using namespace boost::ut;
using namespace saucer::tests;

suite<"smartview"> smartview_suite = []
{
    "expose/evaluate"_test_async = [](saucer::smartview<> &webview)
    {
        webview.expose("test1", [](int value) { return value; });
        webview.expose("test2", [](const std::string &value) -> std::expected<int, std::string> { return std::unexpected{value}; });

        webview.expose("test3",
                       [](int value, const saucer::executor<int, std::string> &exec)
                       {
                           const auto &[resolve, reject] = exec;

                           if (value < 0)
                           {
                               return reject("negative");
                           }

                           return resolve(value);
                       });
        webview.expose("test4",
                       [](int value, const saucer::executor<int, std::string> &exec)
                       {
                           std::thread thread{[=]
                                              {
                                                  const auto &[resolve, reject] = exec;

                                                  std::this_thread::sleep_for(std::chrono::seconds(1));

                                                  if (value < 0)
                                                  {
                                                      return reject("negative");
                                                  }

                                                  return resolve(value);
                                              }};

                           thread.detach();
                       });

        webview.set_url("https://saucer.app");

        expect(webview.evaluate<int>("10 + 5").get() == 15);
        expect(webview.evaluate<std::string>("'Hello' + ' ' + 'World'").get() == "Hello World");

        expect(webview.evaluate<int>("{} + {}", 1, 2).get() == 3);
        expect(webview.evaluate<std::string>("{} + {}", "C++", "23").get() == "C++23");
        expect(webview.evaluate<std::array<int, 2>>("Array.of({})", saucer::make_args(1, 2)).get() == std::array<int, 2>{1, 2});

        expect(eq(webview.evaluate<int>("await saucer.exposed.test1({})", 10).get(), 10));
        expect(webview.evaluate<std::string>("await saucer.exposed.test2({}).then(() => {{}}, err => err)", "test").get() == "test");

        expect(eq(webview.evaluate<int>("await saucer.exposed.test3({})", 10).get(), 10));
        expect(webview.evaluate<std::string>("await saucer.exposed.test3({}).then(() => {{}}, err => err)", -10).get() == "negative");

        expect(eq(webview.evaluate<int>("await saucer.exposed.test4({})", 10).get(), 10));
        expect(webview.evaluate<std::string>("await saucer.exposed.test4({}).then(() => {{}}, err => err)", -10).get() == "negative");
    };
};
