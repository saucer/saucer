#include "cfg.hpp"

#include <saucer/webview.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

void tests(saucer::webview &webview)
{
    std::string last_url{};
    webview.on<saucer::web_event::url_changed>([&](const auto &url) { last_url = url; });

    bool dom_ready{false};
    webview.on<saucer::web_event::dom_ready>([&]() { dom_ready = true; });

    bool load_started{false};
    webview.on<saucer::web_event::load_started>([&]() { load_started = true; });

    bool load_finished{false};
    webview.on<saucer::web_event::load_finished>([&]() { load_finished = true; });

    "background"_test = [&]()
    {
        webview.set_page_background({0, 255, 0, 255});

        auto [r, g, b, a] = webview.page_background();
        expect(r == 0 && g == 255 && b == 0 && a == 255);
    };

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
        {
            auto guard = test::load_guard{webview, load_finished};
            webview.set_url("https://github.com/saucer/saucer");
        }

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
        {
            auto guard = test::load_guard{webview, load_finished};
            webview.set_url("https://saucer.github.io");
        }

        auto title = webview.page_title();
        expect(title == "Saucer | Saucer") << title;
    };
#endif

    "embed"_test = [&]()
    {
        const std::string page = R"html(
        <!DOCTYPE html>
        <html>
            <head>
                <title>Embedded</title>
                <script>
                    location.href = 'https://isocpp.org';
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

        {
            auto guard = test::url_guard{webview, "isocpp"};
            webview.serve("index.html");
        }

        expect(webview.url().contains("isocpp"));
        webview.clear_embedded("index.html");

        {
            auto guard = test::load_guard{webview, load_finished};
            webview.set_url("https://github.com");
        }

        {
            auto guard = test::url_guard{webview, "isocpp", std::chrono::seconds{5}};
            webview.serve("index.html");
        }

        const auto url = webview.url();
        expect(not url.contains("isocpp")) << url;
    };

    "embed_lazy"_test = [&]()
    {
        std::string page = R"html(
        <!DOCTYPE html>
        <html>
            <head>
                <title>Lazy Embedded</title>
                <script>
                    location.href = 'https://isocpp.org';
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

        {
            auto guard = test::url_guard{webview, "isocpp"};
            webview.serve("index.html");
        }

        expect(called == 1);
        expect(webview.url().contains("isocpp"));

        {
            auto guard = test::url_guard{webview, "github"};
            webview.set_url("https://github.com");
        }

        {
            auto guard      = test::url_guard{webview, "isocpp"};
            auto load_guard = test::load_guard{webview, load_finished};

            webview.serve("index.html");
        }

        expect(called == 1);
        expect(webview.url().contains("isocpp"));
    };

    "execute"_test = [&]()
    {
        {
            auto guard = test::load_guard{webview, load_finished};
            webview.set_url("https://saucer.github.io/");
        }

        {
            auto guard = test::load_guard{webview, load_finished};
            webview.execute("location.href = 'https://github.com'");
        }

        expect(webview.url().contains("github"));
    };

    "inject"_test = [&]()
    {
        webview.inject("if (location.href.includes('google')) { location.href = 'https://isocpp.org'; }",
                       saucer::load_time::creation);

        {
            auto guard = test::url_guard{webview, "isocpp"};
            webview.set_url("https://google.com/");
        }

        webview.clear_scripts();
        expect(webview.url().contains("isocpp"));

        webview.inject("location.href = 'https://github.com'", saucer::load_time::ready);

        {
            auto guard      = test::url_guard{webview, "github"};
            auto load_guard = test::load_guard{webview, load_finished};

            webview.set_url("https://google.com/");
        }

        webview.clear_scripts();
        expect(webview.url().contains("github"));
    };

    "scheme"_test = [&]()
    {
        webview.handle_scheme("test",
                              [](const saucer::request &req) -> saucer::scheme_handler::result_type
                              {
                                  expect(req.url() == "test:/index.html");
                                  expect(req.method() == "GET");

                                  const std::string html = R"html(
                                  <!DOCTYPE html>
                                  <html>
                                      <head>
                                        <title>Custom Scheme</title>
                                        <script>
                                            location.href = 'https://isocpp.org';
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

        {
            auto guard      = test::url_guard{webview, "isocpp"};
            auto load_guard = test::load_guard{webview, load_finished};

            webview.serve("index.html", "test");
        }

        webview.remove_scheme("test");

        const auto url = webview.url();
        expect(url.contains("isocpp")) << url;
    };

    webview.clear(saucer::web_event::load_started);
    webview.clear(saucer::web_event::load_finished);

    webview.clear(saucer::web_event::dom_ready);
    webview.clear(saucer::web_event::url_changed);
}

suite<"webview"> webview_suite = []
{
    saucer::webview webview{{.hardware_acceleration = false}};

    const std::jthread thread{[&]()
                              {
                                  std::this_thread::sleep_for(std::chrono::seconds(2));

                                  tests(webview);
                                  webview.close();
                              }};

    webview.show();
    webview.run();
};
