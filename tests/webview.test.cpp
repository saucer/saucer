#include "cfg.hpp"

#include <chrono>
#include <functional>

#include <saucer/webview.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

class wait_guard
{
    using pred_t = std::function<bool()>;

  private:
    pred_t m_pred;
    std::chrono::milliseconds m_delay;
    std::chrono::milliseconds m_timeout;

  public:
    wait_guard(pred_t pred, std::chrono::milliseconds delay = std::chrono::milliseconds(500),
               std::chrono::milliseconds timeout = std::chrono::seconds(20))
        : m_pred(std::move(pred)), m_delay(delay), m_timeout(timeout)
    {
    }

    ~wait_guard()
    {
        auto start = std::chrono::system_clock::now();

        while (!m_pred())
        {
            auto now = std::chrono::system_clock::now();

            if (now - start >= m_timeout)
            {
                std::cout << "Timeout reached" << std::endl;
                return;
            }

            std::this_thread::sleep_for(m_delay);
        }

        expect(true);
    }
};

struct navigation_guard : wait_guard
{
    navigation_guard(saucer::webview &webview, const std::string &contains,
                     std::chrono::milliseconds delay = std::chrono::milliseconds(500))
        : wait_guard([&webview, contains]() { return webview.url().contains(contains); }, delay)
    {
    }
};

struct title_guard : wait_guard
{
    title_guard(saucer::webview &webview, const std::string &contains,
                std::chrono::milliseconds delay = std::chrono::milliseconds(500))
        : wait_guard([&webview, contains]() { return webview.page_title().contains(contains); }, delay)
    {
    }
};

void tests(saucer::webview &webview)
{
    // Some tests are disabled in the CI under certain circumstances (due to issues
    // related to the non existence of a proper display server - mostly with QT6).

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
        webview.set_background({255, 0, 0, 255});

        auto [r, g, b, a] = webview.background();
        expect(r == 255 && g == 0 && b == 0 && a == 255);
    };

    "url"_test = [&]()
    {
        {
            auto guard = wait_guard{[&]()
                                    {
                                        return load_started && load_finished && dom_ready;
                                    }};
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
            auto guard = title_guard{webview, "Saucer | Saucer"};
            webview.set_url("https://saucer.github.io");
        }

        auto title = webview.page_title();
        expect(title == "Saucer | Saucer") << title;
    };
#endif

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

    "force_dark"_test = [&]()
    {
        webview.set_force_dark_mode(true);
        expect(webview.force_dark_mode());

        webview.set_force_dark_mode(false);
        expect(not webview.force_dark_mode());
    };

    "embed"_test = [&]()
    {
        std::string page = R"html(
        <!DOCTYPE html>
        <html>
            <head>
                <title>Embedded</title>
            </head>
        </html>
        )html";

        webview.embed({{"index.html", saucer::embedded_file{
                                          .content = saucer::make_stash(page),
                                          .mime    = "text/html",
                                      }}});

        {
            auto guard = title_guard{webview, "Embedded"};
            webview.serve("index.html");
        }

        expect(webview.page_title() == "Embedded");
        webview.clear_embedded("index.html");

        {
            auto guard = navigation_guard{webview, "github"};
            webview.set_url("https://github.com");
        }

        {
            auto guard = wait_guard{[&webview]()
                                    {
                                        return webview.page_title() != "Embedded";
                                    }};

            webview.serve("index.html");
        }

        auto title = webview.page_title();
        expect(title != "Embedded") << title;
    };

    "embed_lazy"_test = [&]()
    {
        std::string page = R"html(
        <!DOCTYPE html>
        <html>
            <head>
                <title>Lazy Embedded</title>
            </head>
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
            auto guard = title_guard{webview, "Embedded"};
            webview.serve("index.html");
        }

        expect(called == 1);
        expect(webview.page_title() == "Lazy Embedded");

        {
            auto guard = navigation_guard{webview, "github"};
            webview.set_url("https://github.com");
        }
        {
            auto guard = title_guard{webview, "Embedded"};
            webview.serve("index.html");
        }

        expect(called == 1);
        expect(webview.page_title() == "Lazy Embedded");
    };

    "execute"_test = [&]()
    {
        {
            auto guard = title_guard{webview, "Saucer"};
            webview.set_url("https://saucer.github.io/");
        }

        {
            auto guard = title_guard{webview, "Test"};
            webview.execute("document.title = 'Execute Test'");
        }

        auto title = webview.page_title();
        expect(title == "Execute Test") << title;
    };

    "inject"_test = [&]()
    {
        webview.inject("if (location.href.includes('cppref')) { location.href = 'https://isocpp.org'; }",
                       saucer::load_time::creation);

        {
            auto guard = navigation_guard{webview, "isocpp"};
            webview.set_url("https://cppreference.com/");
        }

        webview.clear_scripts();
        expect(webview.url().contains("isocpp.org"));

        webview.inject("document.title = 'Hi!'", saucer::load_time::ready);

        {
            auto guard = title_guard{webview, "Hi"};
            webview.set_url("https://cppreference.com/");
        }

        expect(webview.url().contains("cppreference.com"));
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto title = webview.page_title();
        expect(title == "Hi!") << title;

        webview.clear_scripts();
    };

    "scheme"_test = [&]()
    {
        webview.handle_scheme("test",
                              [](const saucer::request &req) -> saucer::scheme_handler::result_type
                              {
                                  expect(req.url() == "test:/index.html");
                                  expect(req.method() == "GET");

                                  std::string html = R"html(
                                  <!DOCTYPE html>
                                  <html>
                                      <head>
                                        <title>Custom Scheme</title>
                                      </head>
                                  </html>
                                  )html";

                                  return saucer::response{
                                      .data = saucer::make_stash(html),
                                      .mime = "text/html",
                                  };
                              });

        {
            auto guard = title_guard{webview, "Custom"};
            webview.serve("index.html", "test");
        }

        expect(webview.page_title() == "Custom Scheme") << webview.page_title();
        webview.remove_scheme("test");
    };

    webview.clear(saucer::web_event::load_started);
    webview.clear(saucer::web_event::load_finished);

    webview.clear(saucer::web_event::dom_ready);
    webview.clear(saucer::web_event::url_changed);
}

suite<"webview"> webview_suite = []
{
    saucer::webview webview{{.hardware_acceleration = false}};

    std::jthread thread{[&]()
                        {
                            std::this_thread::sleep_for(std::chrono::seconds(2));

                            tests(webview);
                            webview.close();
                        }};

    webview.show();
    webview.run();
};
