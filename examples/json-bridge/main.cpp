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

    webview.expose("close_sync", [&]() { webview.close(); });
    webview.expose("close_async", [&]() { webview.close(); }, saucer::launch::async);

    webview.set_url("https://github.com");
    webview.set_dev_tools(true);

    webview.show();
    webview.run();

    return 0;
}
