#include <catch2/catch.hpp>
#include <serializers/json.hpp>
#include <smartview.hpp>
#include <thread>

TEST_CASE("Smartview functionality is tested", "[smartview]")
{
    auto smartview = saucer::simple_smartview<saucer::serializers::json>();

    smartview.expose("test", [](int a, const std::string &b) {
        REQUIRE(a == 10);
        REQUIRE(b == "Hello World!");
    });

    smartview.expose(
        "test_async",
        [&](int a, const std::string &b) {
            std::this_thread::sleep_for(std::chrono::seconds(10));

            REQUIRE(a == 10);
            REQUIRE(b == "Hello World!");
            REQUIRE(smartview.eval<int>("Math.pow({}, {})", 2, 2)->get() == 4);

            smartview.exit();
        },
        true);

    smartview.eval<int>("Math.pow({}, {})", 2, 2)->then([](int result) { REQUIRE(result == 4); });
    smartview.eval<int>("Math.pow({})", saucer::make_arguments(2, 2))->then([](int result) { REQUIRE(result == 4); });

    REQUIRE_THROWS(smartview.eval<int>("Math.pow({}, {})", 2, 2)->wait());
    REQUIRE_THROWS(smartview.eval<int>("Math.pow({}, {})", 2, 2)->get() == 4);

    smartview.eval<void>("window.saucer.call({}, [{}, {}])", "test", 10, "Hello World!");
    smartview.eval<void>("window.saucer.call({}, [{}, {}])", "test_async", 10, "Hello World!");

    smartview.set_url("https://ddg.gg");
    smartview.show();
    smartview.run();
}