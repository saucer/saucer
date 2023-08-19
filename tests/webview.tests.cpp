#include <saucer/webview.hpp>
#include <catch2/catch.hpp>
#include <future>
#include <thread>

class test_webview : public saucer::webview
{
};

const unsigned char test_html[97] = {0x3c, 0x21, 0x44, 0x4f, 0x43, 0x54, 0x59, 0x50, 0x45, 0x20, 0x68, 0x74, 0x6d, 0x6c,
                                     0x3e, 0x0a, 0x3c, 0x73, 0x63, 0x72, 0x69, 0x70, 0x74, 0x3e, 0x0a, 0x20, 0x20, 0x20,
                                     0x20, 0x77, 0x69, 0x6e, 0x64, 0x6f, 0x77, 0x2e, 0x6c, 0x6f, 0x63, 0x61, 0x74, 0x69,
                                     0x6f, 0x6e, 0x2e, 0x68, 0x72, 0x65, 0x66, 0x20, 0x3d, 0x20, 0x22, 0x68, 0x74, 0x74,
                                     0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x70, 0x61, 0x67, 0x65,
                                     0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x22, 0x3b, 0x0a, 0x3c, 0x2f, 0x73, 0x63, 0x72, 0x69,
                                     0x70, 0x74, 0x3e, 0x0a, 0x3c, 0x2f, 0x68, 0x74, 0x6d, 0x6c, 0x3e, 0x20, 0x0a};

TEST_CASE("Webview functionality is tested", "[webview]")
{
    auto webview = test_webview();

    webview.set_dev_tools(true);
    REQUIRE(webview.dev_tools());
    webview.set_dev_tools(false);
    REQUIRE_FALSE(webview.dev_tools());

    webview.set_context_menu(true);
    REQUIRE(webview.context_menu());
    webview.set_context_menu(false);
    REQUIRE_FALSE(webview.context_menu());

    webview.on<saucer::web_event::url_changed>([&](const std::string &new_url) {
        static auto call_count = 0u;

        switch (call_count)
        {
        case 0:
            REQUIRE(new_url.find("youtube.com") != std::string::npos);
            REQUIRE(webview.url().find("youtube.com") != std::string::npos);
            break;
        case 1:
            REQUIRE(new_url.find("google.com") != std::string::npos);
            REQUIRE(webview.url().find("google.com") != std::string::npos);
            break;
        case 2:
            REQUIRE(new_url.find("github.com") != std::string::npos);
            REQUIRE(webview.url().find("github.com") != std::string::npos);
            break;
        case 3:
            REQUIRE(new_url.find("youtube.com") != std::string::npos);
            REQUIRE(webview.url().find("youtube.com") != std::string::npos);
            break;
        case 5:
            REQUIRE(new_url.find("startpage.com") != std::string::npos);
            REQUIRE(webview.url().find("startpage.com") != std::string::npos);
            break;
        }

        if (call_count == 0)
        {
            auto fut_ptr = std::make_shared<std::future<void>>();
            *fut_ptr = std::async(std::launch::async, [fut_ptr, &webview] {
                //? Now we'll test thread safe functions
                std::this_thread::sleep_for(std::chrono::seconds(2));

                REQUIRE(webview.url().find("youtube.com") != std::string::npos);

                webview.set_dev_tools(true);
                REQUIRE(webview.dev_tools());
                webview.set_dev_tools(false);
                REQUIRE_FALSE(webview.dev_tools());

                webview.set_context_menu(true);
                REQUIRE(webview.context_menu());
                webview.set_context_menu(false);
                REQUIRE_FALSE(webview.context_menu());

                webview.inject(R"(
                        if(window.location.href == "https://www.google.com/")
                        {
                            window.location.href = "https://github.com";
                        }
                    )",
                               saucer::load_time::creation);

                webview.set_url("https://www.google.com/");

                std::this_thread::sleep_for(std::chrono::seconds(5));

                webview.run_java_script("window.location.href = 'https://youtube.com/';");

                std::this_thread::sleep_for(std::chrono::seconds(5));
                webview.clear_embedded();

                auto file = saucer::embedded_file{"text/html", 97, static_cast<const std::uint8_t *>(test_html)};
                webview.embed({{"test.html", file}});
                webview.serve("test.html");

                std::this_thread::sleep_for(std::chrono::seconds(10));
                webview.clear_scripts();
                webview.clear_embedded();
                webview.close();
            });
        }

        call_count++;
    });

    webview.set_url("https://www.youtube.com/");
    webview.show();
    webview.run();
}