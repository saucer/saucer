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

    std::cout << "Main thread: " << std::this_thread::get_id() << std::endl;

    webview.expose("sync_func", [](int param) { return param * 10; });
    webview.expose("async_func", another_func, saucer::launch::async);

    webview.expose("sync_manual_func",
                   [](const custom_data &param, const saucer::executor<int> &exec)
                   {
                       const auto &[resolve, reject] = exec;

                       if (param.field == 1337)
                       {
                           return reject("intruder alert!");
                       }

                       resolve(param.field * 5);
                   });

    webview.expose(
        "async_manual_func",
        [&](const custom_data &param, const saucer::executor<int> &exec)
        {
            const auto &[resolve, reject] = exec;

            std::this_thread::sleep_for(std::chrono::seconds(2));

            if (param.field != 10)
            {
                return reject("expected magic number");
            }

            resolve(webview.evaluate<int>("{} * 1337", param.field).get());
        },
        saucer::launch::async);

    webview.evaluate<int>("await window.saucer.exposed.sync_func({})", saucer::make_args(10)) |
        then([](auto result) { //
            std::cout << "sync_func: " << result << std::endl;
        });

    webview.evaluate<custom_data>("await window.saucer.call({}, [{}])", "async_func", custom_data{500}) |
        then([](auto result) { //
            std::cout << "async_func: " << result.field << std::endl;
        });

    webview.set_url("https://github.com/saucer/saucer");
    webview.set_dev_tools(true);

    webview.show();
    webview.run();

    return 0;
}
