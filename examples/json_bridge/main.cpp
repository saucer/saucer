#include <bridge/json/bridge.hpp>
#include <bridge/json/promise.hpp>
#include <iostream>
#include <webview.hpp>

int main()
{
    saucer::bridged_webview<saucer::bridge::json> webview;

    webview.set_title("Hello World!");
    webview.set_size(500, 600);
    webview.set_max_size(1000, 1200);
    webview.on_url_changed([](const std::string &url) { std::cout << "New url:" << url << std::endl; });

    webview.call<float>("Math.random")->then([](float result) { std::cout << "Random: " << result << std::endl; });
    webview.call<int>("Math.pow", 5, 2)->then([](int result) { std::cout << "Pow(5,2): " << result << std::endl; });
    webview.call<int>("await window.saucer.call", "test", std::make_tuple(10, "Test", 5.f))->then([](int result) {
        std::cout << "Test result: " << result << std::endl;
    });

    webview.expose("test", [](int _int, const std::string &_string, float _float) {
        std::cout << "Int: " << _int << ", "
                  << "String: " << _string << ", Float: " << _float << std::endl;

        return 1337;
    });

    webview.set_url("https://google.com");
    webview.run();

    return 0;
}