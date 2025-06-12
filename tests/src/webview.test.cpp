#include "test.hpp"
#include "utils.hpp"

using namespace boost::ut;
using namespace saucer::tests;

suite<"webview"> webview_suite = []
{
    "page_title"_test_async = [](auto &webview)
    {
        std::string title{};
        webview.template on<saucer::web_event::title>([&](auto value) { title = value; });

        webview.set_url("https://saucer.app");
        saucer::tests::wait_for([&] { return title.contains("Saucer"); });

        expect(title == "Saucer | Saucer");
        expect(webview.page_title() == "Saucer | Saucer");
    };

    "dev_tools"_test_both = [](auto &webview)
    {
        expect(not webview.dev_tools());

        webview.set_dev_tools(true);
        expect(webview.dev_tools());

        webview.set_dev_tools(false);
        expect(not webview.dev_tools());
    };

    "url"_test_async = [](auto &webview)
    {
        std::string url{};
        webview.template on<saucer::web_event::navigated>([&](auto value) { url = value; });

        std::set<saucer::state> state{};
        webview.template on<saucer::web_event::load>([&](auto value) { state.emplace(value); });

        bool ready{false};
        webview.template on<saucer::web_event::dom_ready>([&] { ready = true; });

        webview.set_url("https://saucer.app");
        saucer::tests::wait_for([&] { return state.size() == 2 && ready; });

        expect(state.size() == 2);
        expect(url.starts_with("https://saucer.app"));
        expect(webview.url().starts_with("https://saucer.app"));
    };

    "context-menu"_test_both = [](auto &webview)
    {
        expect(webview.context_menu());

        webview.set_context_menu(false);
        expect(not webview.context_menu());

        webview.set_context_menu(true);
        expect(webview.context_menu());
    };

    "background"_test_both = [](auto &webview)
    {
        webview.set_background({50, 50, 50, 255});
        expect(webview.background() == std::array<std::uint8_t, 4>{50, 50, 50, 255});
    };

#ifndef SAUCER_QT5
    "force-dark"_test_both = [](auto &webview)
    {
        expect(not webview.force_dark_mode());

        webview.set_force_dark_mode(true);
        expect(webview.force_dark_mode());

        webview.set_force_dark_mode(false);
        expect(not webview.force_dark_mode());
    };
#endif

    "scheme"_test_async = [](auto &webview)
    {
        bool finished{false};
        webview.expose("finish", [&] { finished = true; });

        webview.handle_scheme("test",
                              [](const auto &req)
                              {
                                  expect(req.url().starts_with("test://scheme.html"));
                                  expect(req.method() == "GET");

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

        webview.set_url("test://scheme.html");
        saucer::tests::wait_for([&] { return finished; });

        expect(finished);

        finished = false;
        webview.remove_scheme("test");

        webview.reload();
        saucer::tests::wait_for([&] { return finished; });

        expect(not finished);
    };

    "embed"_test_async = [](auto &webview)
    {
        bool finished{false};
        webview.expose("finish", [&] { finished = true; });

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

        webview.embed({{"embed.html", saucer::embedded_file{
                                          .content = saucer::make_stash(page),
                                          .mime    = "text/html",
                                      }}});

        webview.serve("embed.html");
        saucer::tests::wait_for([&] { return finished; });

        expect(finished);

        finished = false;
        webview.clear_embedded("embed.html");

        webview.reload();
        saucer::tests::wait_for([&] { return finished; });

        expect(not finished);
    };

    "embed_lazy"_test_async = [](auto &webview)
    {
        bool finished{false};
        webview.expose("finish", [&] { finished = true; });

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

        std::size_t called{0};

        webview.embed({{"embed.html", saucer::embedded_file{
                                          .content = saucer::stash<>::lazy(
                                              [&page, &called]
                                              {
                                                  called++;
                                                  return saucer::make_stash(page);
                                              }),
                                          .mime = "text/html",
                                      }}});

        webview.serve("embed.html");
        saucer::tests::wait_for([&] { return finished; });

        expect(called == 1);

        finished = false;

        webview.reload();
        saucer::tests::wait_for([&] { return finished; });

        expect(called == 1);
    };

    "inject"_test_async = [](auto &webview)
    {
        std::set<std::string> received{};
        webview.expose("receive", [&](std::string value) { received.emplace(value); });

        webview.inject(saucer::script{
            .code = "saucer.exposed.receive('creation')",
            .time = saucer::load_time::creation,
        });

        webview.inject(saucer::script{
            .code = "saucer.exposed.receive('ready')",
            .time = saucer::load_time::ready,
        });

        webview.inject(saucer::script{
            .code      = "saucer.exposed.receive('permanent')",
            .time      = saucer::load_time::creation,
            .permanent = true,
        });

        webview.set_url("https://saucer.app");
        saucer::tests::wait_for([&] { return received.size() == 3; });

        expect(received.contains("creation"));
        expect(received.contains("ready"));
        expect(received.contains("permanent"));

        received.clear();
        webview.clear_scripts();

        webview.reload();
        saucer::tests::wait_for([&] { return received.size() == 1; });

        expect(received.size() == 1);
        expect(received.contains("permanent"));
    };

    "execute"_test_async = [](auto &webview)
    {
        webview.set_url("https://github.com");

        webview.execute("location.href = 'https://saucer.app'");
        saucer::tests::wait_for([&] { return webview.url().starts_with("https://saucer.app"); });

        expect(webview.url().starts_with("https://saucer.app"));
    };
};
