#include "test.hpp"
#include "utils.hpp"

using namespace boost::ut;
using namespace saucer::tests;

suite<"webview"> webview_suite = []()
{
    "page-title"_test_async = [](const std::shared_ptr<saucer::smartview<>> &webview)
    {
        std::string last_title{};
        webview->on<saucer::web_event::title>([&last_title](const auto &title) { last_title = title; });

        webview->set_url("https://saucer.github.io");
        wait_for([&last_title]() { return !last_title.empty(); });

        auto title = webview->page_title();

        expect(title == "Saucer | Saucer") << title;
        expect(last_title == title) << last_title;
    };

    "dev-tools"_test_both = [](const auto &webview)
    {
        expect(not webview->dev_tools());

        webview->set_dev_tools(true);
        expect(webview->dev_tools());

        webview->set_dev_tools(false);
        expect(not webview->dev_tools());
    };

    "url"_test_async = [](const std::shared_ptr<saucer::smartview<>> &webview)
    {
        std::string last_url{};
        webview->on<saucer::web_event::navigated>([&last_url](const auto &url) { last_url = url; });

        bool dom_ready{false};
        webview->on<saucer::web_event::dom_ready>([&dom_ready]() { dom_ready = true; });

        std::vector<saucer::state> states;
        webview->on<saucer::web_event::load>([&states](const auto &state) { states.emplace_back(state); });

        webview->set_url("https://github.com/saucer/saucer");
        wait_for([&last_url]() { return !last_url.empty(); });

        auto url = webview->url();

        expect(url.contains("github.com/saucer/saucer")) << url;
        expect(last_url == url) << last_url;

        wait_for([&states]() { return states.size() >= 2; });
        expect(dom_ready);

        expect(states[0] == saucer::state::started);
        expect(states[1] == saucer::state::finished);
    };

    "context-menu"_test_both = [](const auto &webview)
    {
        expect(webview->context_menu());

        webview->set_context_menu(false);
        expect(not webview->context_menu());

        webview->set_context_menu(true);
        expect(webview->context_menu());
    };

    "background"_test_both = [](const auto &webview)
    {
        webview->set_background({50, 50, 50, 255});

        auto [r, g, b, a] = webview->background();
        expect(r == 50 && g == 50 && b == 50 && a == 255);
    };

#ifndef SAUCER_QT5
    "force-dark"_test_both = [](const auto &webview)
    {
        expect(not webview->force_dark_mode());

        webview->set_force_dark_mode(true);
        expect(webview->force_dark_mode());

        webview->set_force_dark_mode(false);
        expect(not webview->force_dark_mode());
    };
#endif

    "scheme"_test_async = [](const auto &webview)
    {
        bool finished{false};
        webview->expose("finish", [&finished]() { finished = true; });

        webview->handle_scheme("test",
                               [](const saucer::scheme::request &req) -> saucer::scheme::handler::result_type
                               {
                                   expect(req.url().starts_with("test://scheme.html")) << req.url();
                                   expect(req.method() == "GET") << req.method();

                                   const std::string html = R"html(
                                    <!DOCTYPE html>
                                    <html>
                                        <head>
                                            <title>Custom Scheme</title>
                                            <script>
                                                saucer.exposed.finish();
                                            </script>
                                        </head>
                                        <body>
                                            Custom Scheme Test
                                        </body>
                                    </html>
                                   )html";

                                   return saucer::scheme::response{
                                       .data = saucer::make_stash(html),
                                       .mime = "text/html",
                                   };
                               });

        webview->set_url("test://scheme.html");

        wait_for(finished);
        expect(finished);

        webview->remove_scheme("test");
        finished = false;

        webview->reload();
        wait_for(finished);

        expect(not finished);
    };

    "embed"_test_async = [](const auto &webview)
    {
        bool finished{false};
        webview->expose("finish", [&finished]() { finished = true; });

        const std::string page = R"html(
            <!DOCTYPE html>
            <html>
                <head>
                    <title>Embedded</title>
                    <script>
                        saucer.exposed.finish();
                    </script>
                </head>
                <body>
                    Embedded Test
                </body>
            </html>
        )html";

        webview->embed({{"embed.html", saucer::embedded_file{
                                           .content = saucer::make_stash(page),
                                           .mime    = "text/html",
                                       }}});

        webview->serve("embed.html");

        wait_for(finished);
        expect(finished);

        webview->clear_embedded("embed.html");
        finished = false;

        webview->reload();
        wait_for(finished);

        expect(not finished);
    };

    "embed_lazy"_test_async = [](const auto &webview)
    {
        bool finished{false};
        webview->expose("finish", [&finished]() { finished = true; });

        const std::string page = R"html(
            <!DOCTYPE html>
            <html>
                <head>
                    <title>Lazy Embedded</title>
                    <script>
                        saucer.exposed.finish();
                    </script>
                </head>
                <body>
                    Lazy Embedded Test
                </body>
            </html>
        )html";

        std::size_t called{};

        webview->embed({{"embed.html", saucer::embedded_file{
                                           .content = saucer::stash<>::lazy(
                                               [&page, &called]()
                                               {
                                                   called++;
                                                   return saucer::make_stash(page);
                                               }),
                                           .mime = "text/html",
                                       }}});

        webview->serve("embed.html");

        wait_for(finished);
        expect(called == 1);

        finished = false;
        webview->reload();

        wait_for(finished);
        expect(called == 1);
    };

    "execute"_test_async = [](const auto &webview)
    {
        webview->set_url("https://google.com");
        webview->execute("location.href = 'https://github.com'");

        wait_for([&webview]() { return webview->url().contains("github"); });
        expect(webview->url().contains("github")) << webview->url();
    };

    "inject"_test_async = [](const auto &webview)
    {
        std::vector<std::string> states;
        webview->expose("push_state", [&](const std::string &state) { states.emplace_back(state); });

        bool finished{false};
        webview->expose("finish", [&]() { finished = true; });

        webview->inject({
            .code = "saucer.exposed.push_state(document.readyState)",
            .time = saucer::load_time::creation,
        });

        webview->inject({
            .code = "saucer.exposed.push_state(document.readyState)",
            .time = saucer::load_time::ready,
        });

        webview->inject({
            .code      = "saucer.exposed.finish()",
            .time      = saucer::load_time::ready,
            .permanent = true,
        });

        webview->set_url("https://saucer.github.io");
        wait_for(finished);

        expect(states.size() == 2);
        expect(states[0] != "complete");
        expect(states[1] != "loading");

        webview->clear_scripts();
        finished = false;

        webview->reload();
        wait_for(finished);

        expect(states.size() == 2);
    };
};
