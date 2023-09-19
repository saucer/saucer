#include "cfg.hpp"

#include <saucer/webview.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

// NOLINTNEXTLINE
suite webview_suite = []
{
    "webview"_test = [&]
    {
        saucer::webview webview({.hardware_acceleration = false});
        std::uint64_t id{};

        "dev_tools"_test = [&]
        {
            webview.set_dev_tools(true);
            expect(webview.dev_tools());

            webview.set_dev_tools(false);
            expect(not webview.dev_tools());
        };

        "context_menu"_test = [&]
        {
            webview.set_context_menu(true);
            expect(webview.context_menu());

            webview.set_context_menu(false);
            expect(not webview.context_menu());
        };

        "navigation"_test = [&]
        {
            webview.set_url("https://www.youtube.com");

            auto callback = [&](const std::string &url)
            {
                expect(eq(url, webview.url()));
                webview.remove(saucer::web_event::url_changed, id);
            };

            id = webview.on<saucer::web_event::url_changed>(callback);
        };

        "scripting"_test = [&]
        {
            webview.inject(R"js(
                if (!window.location.href.includes("github"))
                {
                    window.location = 'https://www.github.com';
                }
            )js",
                           saucer::load_time::ready);

            auto callback = [&](const std::string &url)
            {
                static bool first = false;

                if (url.find("youtube") != std::string::npos)
                {
                    return;
                }

                if (!first)
                {
                    expect(url.find("github") != std::string::npos) << url;

                    first = true;
                    webview.clear_scripts();
                    webview.run_java_script("window.location = 'https://www.google.com'");

                    return;
                }

                expect(url.find("google") != std::string::npos) << url;

                webview.clear(saucer::web_event::url_changed);
                webview.close();
            };

            webview.on<saucer::web_event::url_changed>(callback);
        };

        webview.show();
        webview.run();
    };

    "embedding"_test = [&]
    {
        saucer::webview webview;

        std::array<std::uint8_t, 97> data{
            0x3c, 0x21, 0x44, 0x4f, 0x43, 0x54, 0x59, 0x50, 0x45, 0x20, 0x68, 0x74, 0x6d, 0x6c, 0x3e, 0x0a, 0x3c,
            0x73, 0x63, 0x72, 0x69, 0x70, 0x74, 0x3e, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x77, 0x69, 0x6e, 0x64, 0x6f,
            0x77, 0x2e, 0x6c, 0x6f, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x2e, 0x68, 0x72, 0x65, 0x66, 0x20, 0x3d,
            0x20, 0x22, 0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x70, 0x61,
            0x67, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x22, 0x3b, 0x0a, 0x3c, 0x2f, 0x73, 0x63, 0x72, 0x69, 0x70,
            0x74, 0x3e, 0x0a, 0x3c, 0x2f, 0x68, 0x74, 0x6d, 0x6c, 0x3e, 0x20, 0x0a //
        };

        auto callback = [&](const std::string &url)
        {
            if (url.starts_with("saucer"))
            {
                return;
            }

            expect(url.find("startpage") != std::string::npos) << url;
            webview.close();
        };

        webview.on<saucer::web_event::url_changed>(callback);

        auto file = saucer::embedded_file{"text/html", data};
        webview.embed({{"test.html", file}});
        webview.serve("test.html");

        webview.show();
        webview.run();
    };
};