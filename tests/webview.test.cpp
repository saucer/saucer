#include "cfg.hpp"
#include "saucer/stash/stash.hpp"

#include <saucer/webview.hpp>

#ifdef _WIN32
#include <thread>
#include <chrono>
#endif

using namespace boost::ut;
using namespace boost::ut::literals;

suite webview_suite = []
{
    "webview"_test = [&]
    {
#ifdef _WIN32
        //? Creating the WebView on windows seems to sometimes not fire the created callback when they're created
        //? rapidly (seems to be a WebView2 issue)

        std::this_thread::sleep_for(std::chrono::seconds(2));
#endif

        saucer::webview webview({.hardware_acceleration = false});
        std::uint64_t id{};

        "background"_test = [&]
        {
            auto color = webview.background();

            expect(eq(color.at(0), 255));
            expect(eq(color.at(1), 255));
            expect(eq(color.at(2), 255));
            expect(eq(color.at(3), 255));

            webview.set_background({255, 0, 0, 255});
            color = webview.background();

            expect(eq(color.at(0), 255));
            expect(eq(color.at(1), 0));
            expect(eq(color.at(2), 0));
            expect(eq(color.at(3), 255));
        };

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
            webview.set_url("https://www.wikipedia.com");

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
                if (!window.location.href.includes("kernel"))
                {
                    window.location = 'https://kernel.org/';
                }
            )js",
                           saucer::load_time::ready);

            auto callback = [&](const std::string &url)
            {
                static bool first = false;

                if (url.find("wikipedia") != std::string::npos)
                {
                    return;
                }

                if (!first)
                {
                    expect(url.find("kernel") != std::string::npos) << url;

                    first = true;
                    webview.clear_scripts();
                    webview.execute("window.location = 'https://isocpp.org/'");

                    return;
                }

                expect(url.find("isocpp") != std::string::npos) << url;

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
#ifdef _WIN32
        std::this_thread::sleep_for(std::chrono::seconds(2));
#endif
        saucer::webview webview;

        std::string html = R"html(
        <!DOCTYPE html>
        <html>
            <head>
                <title>Redirect</title>
            </head>
            <body>
                <script>
                    window.location.href = "https://startpage.com/";
                </script>
            </body>
        </html> 
        )html";

        auto callback = [&](const std::string &url)
        {
            if (url.empty() || url.find("saucer") != std::string::npos)
            {
                return;
            }

            expect(url.find("startpage") != std::string::npos);
            webview.close();
        };

        webview.on<saucer::web_event::url_changed>(callback);

        auto file = saucer::embedded_file{saucer::make_stash(html), "text/html"};
        webview.embed({{"test.html", file}});
        webview.serve("test.html");

        webview.show();
        webview.run();
    };
};
