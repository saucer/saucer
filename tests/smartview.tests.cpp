#include <saucer/serializers/json.hpp>
#include <saucer/utils/future.hpp>
#include <saucer/smartview.hpp>
#include <catch2/catch.hpp>
#include <thread>

TEST_CASE("Smartview functionality is tested", "[smartview]")
{
    auto smartview = saucer::simple_smartview<saucer::serializers::json>();

    bool test_called = false;
    smartview.expose("test", [&]() { test_called = true; });

    smartview.expose("test2", [](int a, const std::string &b) {
        REQUIRE(a == 10);
        REQUIRE(b == "Hello World!");
    });

    smartview.expose(
        "test_async",
        [&](int a, const std::string &b) {
            std::this_thread::sleep_for(std::chrono::seconds(10));

            REQUIRE(a == 10);
            REQUIRE(b == "Hello World!");
            REQUIRE(smartview.eval<int>("Math.pow({}, {})", 2, 2).get() == 4);

            smartview.eval<void>("window.saucer.call({}, [])", "test").wait();
            REQUIRE(test_called);

            smartview.close();
        },
        true);

    smartview.eval<int>("Math.pow({}, {})", 2, 2) | saucer::then([](int result) { //
        REQUIRE(result == 4);
    });

    smartview.eval<int>("Math.pow({})", saucer::make_args(2, 2)) | saucer::then([](int result) { //
        REQUIRE(result == 4);
    });

    smartview.eval<void>("window.saucer.call({}, [{}, {}])", "test2", 10, "Hello World!") | saucer::forget();
    smartview.eval<void>("window.saucer.call({}, [{}, {}])", "test_async", 10, "Hello World!") | saucer::forget();

    smartview.set_url("https://google.com");
    smartview.show();
    smartview.run();
}