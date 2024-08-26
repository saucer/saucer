#include "cfg.hpp"

#include <saucer/smartview.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

static void tests(saucer::smartview<> &webview)
{
    std::string last_title{};
    webview.on<saucer::web_event::title_changed>([&](const auto &title) { last_title = title; });

    std::string last_url{};
    webview.on<saucer::web_event::url_changed>([&](const auto &url) { last_url = url; });

    bool load_finished{false};
    webview.on<saucer::web_event::load_finished>([&]() { load_finished = true; });

    bool dom_ready{false};
    webview.on<saucer::web_event::dom_ready>([&]() { dom_ready = true; });

    bool load_started{false};
    webview.on<saucer::web_event::load_started>(
        [&]()
        {
            load_started  = true;
            dom_ready     = false;
            load_finished = false;
        });

    "dev_tools"_test = [&]()
    {
        expect(not webview.dev_tools());
        webview.set_dev_tools(true);

        expect(webview.dev_tools());
        webview.set_dev_tools(false);

        expect(not webview.dev_tools());
    };

    "context_menu"_test = [&]()
    {
        webview.set_context_menu(true);
        expect(webview.context_menu());

        webview.set_context_menu(false);
        expect(not webview.context_menu());
    };

#ifndef SAUCER_QT5
    "force_dark"_test = [&]()
    {
        webview.set_force_dark_mode(true);
        expect(webview.force_dark_mode());

        webview.set_force_dark_mode(false);
        expect(not webview.force_dark_mode());
    };
#endif

    "url"_test = [&]()
    {
        webview.set_url("https://github.com/saucer/saucer");
        test::wait_for(load_finished);

        expect(load_started);
        expect(load_finished);

#ifndef SAUCER_CI
        expect(dom_ready);
#endif

        expect(webview.url() == last_url) << webview.url() << ":" << last_url;
        expect(webview.url().contains("github.com/saucer/saucer")) << webview.url();
    };

#ifndef SAUCER_CI
    "page_title"_test = [&]()
    {
        last_title.clear();

        webview.set_url("https://saucer.github.io");
        test::wait_for([&]() { return !last_title.empty(); });

        auto title = webview.page_title();
        expect(title == "Saucer | Saucer") << title;
    };
#endif

    "embed"_test = [&]()
    {
        bool done{false};
        webview.expose("embed_done", [&done]() { done = true; });

        const std::string page = R"html(
        <!DOCTYPE html>
        <html>
            <head>
                <title>Embedded</title>
                <script>
                    window.saucer.exposed.embed_done();
                </script>
            </head>
            <body>
                Embedded Test
            </body>
        </html>
        )html";

        webview.embed({{"index.html", saucer::embedded_file{
                                          .content = saucer::make_stash(page),
                                          .mime    = "text/html",
                                      }}});

        webview.serve("index.html");
        test::wait_for(done);

        expect(done);
        done = false;

        webview.clear_embedded("index.html");

        webview.set_url("https://github.com");
        test::wait_for(load_finished);

        webview.serve("index.html");
        test::wait_for(done, std::chrono::seconds(5));

        expect(not done);
    };

    "embed_lazy"_test = [&]()
    {
        bool done{false};
        webview.expose("lazy_done", [&done]() { done = true; });

        std::string page = R"html(
        <!DOCTYPE html>
        <html>
            <head>
                <title>Lazy Embedded</title>
                <script>
                    window.saucer.exposed.lazy_done();
                </script>
            </head>
            <body>
                Lazy Embed Test
            </body>
        </html>
        )html";

        std::size_t called = 0;

        webview.embed({{"index.html", saucer::embedded_file{
                                          .content = saucer::stash<>::lazy(
                                              [&page, &called]()
                                              {
                                                  called++;
                                                  return saucer::make_stash(page);
                                              }),
                                          .mime = "text/html",
                                      }}});

        webview.serve("index.html");
        test::wait_for(done);

        expect(done);
        expect(called == 1);

        done = false;

        webview.set_url("https://github.com");
        test::wait_for(load_finished);

        webview.serve("index.html");
        test::wait_for(done);

        expect(done);
        expect(called == 1);
    };

    "execute"_test = [&]()
    {
        webview.execute("location.href = 'https://isocpp.org'");

        test::wait_for([&webview]() { return webview.url().contains("isocpp"); });
        expect(webview.url().contains("isocpp"));
    };

    "inject"_test = [&]()
    {
        using enum saucer::load_time;

        std::string state;
        webview.expose("inject", [&state](std::string _state) { state = std::move(_state); });

        webview.inject({.code = "window.saucer.exposed.inject(document.readyState)", .time = creation});
        webview.set_url("https://saucer.github.io");

        test::wait_for([&state]() { return !state.empty(); });
        expect(!state.empty() && state != "complete") << state;

        webview.clear_scripts();
        state.clear();

        webview.inject({.code = "window.saucer.exposed.inject(document.readyState)", .time = ready});
        webview.set_url("https://github.com");

        test::wait_for([&state]() { return !state.empty(); });
        expect(!state.empty() && state != "loading") << state;

        webview.clear_scripts();
    };

    "scheme"_test = [&]()
    {
        bool done{false};
        webview.expose("scheme_done", [&done]() { done = true; });

        webview.handle_scheme("test",
                              [](const saucer::request &req) -> saucer::scheme_handler::result_type
                              {
                                  expect(req.url().starts_with("test://index.html")) << req.url();
                                  expect(req.method() == "GET") << req.method();

                                  const std::string html = R"html(
                                  <!DOCTYPE html>
                                  <html>
                                      <head>
                                        <title>Custom Scheme</title>
                                        <script>
                                            saucer.exposed.scheme_done();
                                        </script>
                                      </head>
                                      <body>
                                        Custom Scheme Test
                                      </body>
                                  </html>
                                  )html";

                                  return saucer::response{
                                      .data = saucer::make_stash(html),
                                      .mime = "text/html",
                                  };
                              });

        webview.set_url("test://index.html");
        test::wait_for(done);

        expect(done);

        webview.remove_scheme("test");
        done = false;

        webview.set_url("https://github.com");
        test::wait_for(load_finished);

        webview.set_url("test://index.html");
        test::wait_for(done, std::chrono::seconds(5));

        expect(not done);
    };

    webview.clear(saucer::web_event::load_started);
    webview.clear(saucer::web_event::load_finished);

    webview.clear(saucer::web_event::dom_ready);
    webview.clear(saucer::web_event::url_changed);
}

suite<"webview"> webview_suite = []
{
    saucer::smartview webview{{.hardware_acceleration = false}};

    webview.show();

    const std::jthread thread{[&]()
                              {
                                  tests(webview);
                                  webview.close();
                              }};

    webview.run();
};
