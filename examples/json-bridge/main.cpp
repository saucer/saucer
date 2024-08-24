#include <print>
#include <thread>

#include <saucer/smartview.hpp>
#include <saucer/utils/future.hpp>

struct custom_data
{
    int field;
};

auto another_func(const custom_data &data)
{
    std::println("Called from thread: {}", std::this_thread::get_id());
    return custom_data{data.field * 2};
}

int main()
{
    using saucer::then;
    saucer::smartview webview;

    webview.set_title("Hello World!");
    webview.set_size(500, 600);

    webview.set_min_size(400, 500);
    webview.set_max_size(1000, 1200);

    webview.on<saucer::web_event::url_changed>([](const auto &url) { //
        std::println("New url: {}", url);
    });

    webview.on<saucer::window_event::resize>([](auto width, auto height) { //
        std::println("Resize {}:{}", width, height);
    });

    webview.evaluate<float>("Math.random()") | then([](auto result) { //
        std::println("Random: {}", result);
    });

    webview.evaluate<int>("Math.pow({},{})", 5, 2) | then([](auto result) { //
        std::println("Pow(5,2): {}", result);
    });

    webview.expose("some_func", [](int param) { return param * 10; });

    webview.evaluate<int>("await window.saucer.call({})", saucer::make_args("some_func", std::make_tuple(10))) |
        then([](auto result) { std::println("some_func: {}", result); });

    std::println("Main thread: {}", std::this_thread::get_id());
    webview.expose("another_func", another_func, saucer::launch::async);

    webview.evaluate<custom_data>("await window.saucer.call({}, [{}])", "another_func", custom_data{500}) |
        then([](auto result) { std::println("another_func: {}", result.field); });

    webview.set_url("https://github.com/saucer/saucer");
    webview.set_dev_tools(true);

    webview.show();
    webview.run();

    return 0;
}
