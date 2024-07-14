#include <thread>
#include <iostream>

#include <saucer/smartview.hpp>
#include <saucer/utils/future.hpp>

struct custom_data
{
    int field;
};

auto another_func(const custom_data &data)
{
    std::cout << "Called from thread: " << std::this_thread::get_id() << std::endl;
    std::cout << "Sleeping 5 seconds..." << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(5));

    return custom_data{data.field * 2};
}

int main()
{
    using saucer::launch;
    using saucer::then;

    saucer::smartview webview;

    webview.set_title("Hello World!");
    webview.set_size(500, 600);

    webview.set_min_size(400, 500);
    webview.set_max_size(1000, 1200);

    webview.on<saucer::web_event::url_changed>([](const auto &url) { //
        std::cout << "New url:" << url << std::endl;
    });

    webview.on<saucer::window_event::resize>([](auto width, auto height) { //
        std::cout << width << ":" << height << std::endl;
    });

    webview.evaluate<float>("Math.random()") | then([](auto result) { //
        std::cout << "Random: " << result << std::endl;
    });

    webview.evaluate<int>("Math.pow({},{})", 5, 2) | then([](auto result) { //
        std::cout << "Pow(5,2): " << result << std::endl;
    });

    webview.expose("some_func", [](int param) { return param * 10; });

    webview.evaluate<int>("await window.saucer.exposed.some_func({})", saucer::make_args(10)) |
        then([](auto result) { //
            std::cout << "some_func: " << result << std::endl;
        });

    std::cout << "Main thread: " << std::this_thread::get_id() << std::endl;

    webview.expose<launch::manual>("another_func",
                                   [](const custom_data &data, const saucer::executor<custom_data> &exec)
                                   {
                                       const auto &[resolve, reject] = exec;

                                       std::thread{[resolve, data]()
                                                   {
                                                       resolve(another_func(data));
                                                   }}
                                           .detach();
                                   });

    webview.evaluate<custom_data>("await window.saucer.call({}, [{}])", "another_func", custom_data{500}) |
        then([](auto result) { //
            std::cout << "another_func: " << result.field << std::endl;
        });

    webview.set_url("https://github.com/saucer/saucer");
    webview.set_dev_tools(true);

    webview.show();
    webview.run();

    return 0;
}
