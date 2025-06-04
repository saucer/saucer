#include <print>

#include <saucer/smartview.hpp>

struct custom_data
{
    int field;
};

coco::stray start(saucer::application *app)
{
    auto webview = saucer::smartview{{
        .application = app,
    }};

    webview.set_title("Hello World!");
    webview.set_size(500, 600);

    webview.set_min_size(400, 500);
    webview.set_max_size(1000, 1200);

    // Add event callbacks

    webview.on<saucer::web_event::navigated>([](const auto &url) { //
        std::println("New url: {}", url);
    });

    webview.on<saucer::window_event::resize>([](auto width, auto height) { //
        std::println("Resize {}:{}", width, height);
    });

    // Expose Functions

    webview.expose("func1",
                   [](int a, int b)
                   { //
                       return a + b;
                   });

    webview.expose("func2",
                   [](int a, int b) -> std::expected<int, std::string>
                   {
                       if (a < 0 || b < 0)
                       {
                           return std::unexpected<std::string>{"Negative values not allowed!"};
                       }

                       return a + b;
                   });

    webview.expose("func3", [&](int a, int b) -> coco::task<double>
                   { //
                       co_return co_await webview.evaluate<double>("Math.pow({}, {})", a, b);
                   });

    webview.expose("func4",
                   [&](int a, int b) -> coco::task<std::expected<double, std::string>>
                   {
                       if (a < 0 || b < 0)
                       {
                           co_return std::unexpected<std::string>{"Negative values not allowed!"};
                       }

                       co_return co_await webview.evaluate<double>("Math.pow({}, {})", a, b);
                   });

    webview.expose("func5",
                   [&](int a, int b, const saucer::executor<double, std::string> &executor)
                   {
                       if (a < 0 || b < 0)
                       {
                           executor.reject("Negative values not allowed!");
                           return;
                       }

                       coco::then(webview.evaluate<double>("Math.pow({}, {})", a, b),
                                  [executor](auto result) { executor.resolve(result); });
                   });

    webview.expose("func6",
                   [&](custom_data data, saucer::executor<int, void> executor)
                   {
                       auto thread = std::thread{[](auto data, auto executor)
                                                 {
                                                     std::this_thread::sleep_for(std::chrono::milliseconds(data.field));
                                                     executor.resolve(data.field * 2);
                                                 },
                                                 data, std::move(executor)};

                       thread.detach();
                   });

    // Set the URL, Show the Dev-Tools and run

    webview.set_url("https://github.com/saucer/saucer");
    webview.set_dev_tools(true);

    webview.show();

    // Evaluate JavaScript code and capture the result

    std::println("Random: {}", co_await webview.evaluate<float>("Math.random()"));
    std::println("Pow(5,2): {}", co_await webview.evaluate<int>("Math.pow({},{})", 5, 2));

    std::println("Func1: {}", co_await webview.evaluate<int>("await saucer.exposed.func1({}, {})", 1, 2));
    std::println("Func2: {}", co_await webview.evaluate<int>("await saucer.exposed.func2({})", saucer::make_args(2, 3)));

    std::println("Func3: {}", co_await webview.evaluate<double>("await saucer.exposed.func3({}, {})", 3, 4));
    std::println("Func4: {}", co_await webview.evaluate<double>("await saucer.exposed.func4({}, {})", 4, 5));
    std::println("Func5: {}", co_await webview.evaluate<double>("await saucer.exposed.func5({}, {})", 5, 6));

    std::println("Func6: {}", co_await webview.evaluate<int>("await saucer.exposed.func6({})", custom_data{10}));

    co_await app->finish(); // Is resolved when the last window is closed
}

int main()
{
    return saucer::application::create({.id = "example"})->run(start);
}
