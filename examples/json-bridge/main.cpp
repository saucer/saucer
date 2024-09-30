#include <saucer/smartview.hpp>
#include <saucer/utils/future.hpp>

#include <print>

struct custom_data
{
    int field;
};

auto func2(const custom_data &data)
{
    return custom_data{data.field * 2};
}

int main()
{
    using saucer::then;

    auto app = saucer::application::acquire({
        .id = "example",
    });

    saucer::smartview webview{{
        .application = app,
    }};

    webview.set_title("Hello World!");
    webview.set_size(500, 600);

    webview.set_min_size(400, 500);
    webview.set_max_size(1000, 1200);

    webview.on<saucer::web_event::navigated>([](const auto &url) { //
        std::println("New url: {}", url);
    });

    webview.on<saucer::window_event::resize>([](auto width, auto height) { //
        std::println("Resize {}:{}", width, height);
    });

    webview.expose("func1", [](int a, int b) { return a + b; });
    webview.expose("func2", func2, saucer::launch::async);

    webview.evaluate<float>("Math.random()") | then([](auto result) { //
        std::println("Random: {}", result);
    });

    webview.evaluate<int>("Math.pow({},{})", 5, 2) | then([](auto result) { //
        std::println("Pow(5,2): {}", result);
    });

    webview.evaluate<int>("await saucer.exposed.func1({})", saucer::make_args(5, 5)) | then([](auto res) { //
        std::println("func1: {}", res);
    });

    webview.evaluate<custom_data>("await saucer.exposed.func2({})", custom_data{500}) | then([](auto res) { //
        std::println("func2: {}", res.field);
    });

    webview.set_url("https://github.com/saucer/saucer");
    webview.set_dev_tools(true);

    webview.show();
    app->run();

    return 0;
}
