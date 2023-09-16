#include <iostream>
#include <saucer/smartview.hpp>
#include <saucer/utils/future.hpp>
#include <thread>

struct custom_data
{
    int field;
};

template <>
struct glz::meta<custom_data>
{
    using T = custom_data;
    static constexpr auto value = object( //
        "field", &T::field                //
    );
};

auto another_func(const custom_data &data)
{
    std::cout << "Called from thread: " << std::this_thread::get_id() << std::endl;
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

    webview.evaluate<int>("await window.saucer.call({})", saucer::make_args("some_func", std::make_tuple(10))) |
        then([](auto result) { //
            std::cout << "some_func: " << result << std::endl;
        });

    std::cout << "Main thread: " << std::this_thread::get_id() << std::endl;
    webview.expose("another_func", another_func, true);

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