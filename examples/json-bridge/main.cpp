#include <iostream>
#include <saucer/smartview.hpp>
#include <saucer/utils/future.hpp>
#include <saucer/serializers/json.hpp>

int main()
{
    using saucer::then;
    saucer::simple_smartview<saucer::serializers::json> webview;

    webview.set_title("Hello World!");
    webview.set_size(500, 600);

    webview.set_min_size(400, 500);
    webview.set_max_size(1000, 1200);

    webview.on<saucer::web_event::url_changed>([](const std::string &url) { std::cout << "New url:" << url << std::endl; });
    webview.on<saucer::window_event::resize>([](std::size_t width, std::size_t height) { std::cout << width << ":" << height << std::endl; });

    webview.eval<float>("Math.random()") | then([](float result) { std::cout << "Random: " << result << std::endl; });
    webview.eval<int>("Math.pow({},{})", 5, 2) | then([](int result) { std::cout << "Pow(5,2): " << result << std::endl; });

    webview.expose("test", [](int _int, const std::string &_string, float _float) {
        std::cout << "Int: " << _int << ", "
                  << "String: " << _string << ", Float: " << _float << std::endl;

        return 1337;
    });

    webview.eval<int>("await window.saucer.call({}, {})", "test", std::make_tuple(10, "Test", 5.f)) | then([](int result) { std::cout << "Test result: " << result << std::endl; });

    webview.expose(
        "test_async",
        [&](int test) {
            auto rtn = webview.eval<int>("Math.pow({},{})", test, 3).get();
            std::cout << "Rtn: " << rtn << std::endl;
            return rtn;
        },
        true);

    webview.eval<int>("await window.saucer.call({}, {})", "test_async", std::make_tuple(100)) | then([](int result) { std::cout << "Test result: " << result << std::endl; });

    webview.set_url("file://test.html");
    webview.set_context_menu(false);
    webview.set_dev_tools(true);
    webview.show();
    webview.run();

    return 0;
}