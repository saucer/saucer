#include "cfg.hpp"

#include <chrono>
#include <atomic>

#include <saucer/webview.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

class navigation_guard
{
    std::shared_ptr<std::promise<void>> m_ready;
    std::shared_ptr<std::promise<void>> m_finished;

  private:
    std::chrono::seconds m_timeout;

  public:
    navigation_guard(saucer::webview &webview, std::chrono::seconds timeout = std::chrono::seconds(20))
        : m_ready(std::make_shared<std::promise<void>>()), m_finished(std::make_shared<std::promise<void>>()),
          m_timeout(timeout)
    {
        webview.once<saucer::web_event::dom_ready>([m_ready = m_ready]() { m_ready->set_value(); });
        webview.once<saucer::web_event::load_finished>([m_finished = m_finished]() { m_finished->set_value(); });
    }

    ~navigation_guard()
    {
        m_ready->get_future().wait_for(m_timeout);
        m_finished->get_future().wait_for(m_timeout);
    }
};

class title_guard
{
    std::shared_ptr<std::promise<void>> m_promise;

  private:
    std::chrono::seconds m_timeout;

  public:
    title_guard(saucer::webview &webview, std::chrono::seconds timeout = std::chrono::seconds(20))
        : m_promise(std::make_shared<std::promise<void>>()), m_timeout(timeout)
    {
        webview.once<saucer::web_event::title_changed>([m_promise = m_promise](auto...) { m_promise->set_value(); });
    }

    ~title_guard()
    {
        m_promise->get_future().wait_for(m_timeout);
    }
};

void tests(saucer::webview &webview)
{
    // Some tests have been disabled on QT6 due to upstream bugs which are yet to be fixed.

    std::string last_url{};
    webview.on<saucer::web_event::url_changed>([&](const auto &url) { last_url = url; });

    bool dom_ready{false};
    webview.on<saucer::web_event::dom_ready>([&]() { dom_ready = true; });

    bool load_started{false};
    webview.on<saucer::web_event::load_started>([&]() { load_started = true; });

    bool load_finished{false};
    webview.on<saucer::web_event::load_finished>([&]() { load_finished = true; });

#ifndef SAUCER_QT6
    "background"_test = [&]()
    {
        webview.set_background({255, 0, 0, 255});

        auto [r, g, b, a] = webview.background();
        expect(r == 255 && g == 0 && b == 0 && a == 255);
    };
#endif

    "url"_test = [&]()
    {
        {
            auto guard = navigation_guard{webview};
            webview.set_url("https://github.com/saucer/saucer");
        }

        expect(load_started);
        expect(load_finished);
        expect(dom_ready);

        expect(webview.url() == last_url) << webview.url() << ":" << last_url;
        expect(webview.url().find("github.com/saucer/saucer") != std::string::npos) << webview.url();
    };

    "page_title"_test = [&]()
    {
        {
            auto nav_guard = navigation_guard{webview};
            auto tit_guard = title_guard{webview};

            webview.set_url("https://saucer.github.io");
        }

        auto title = webview.page_title();
        expect(title == "Saucer | Saucer") << title;
    };

#ifndef SAUCER_QT6
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
#endif

    "embed"_test = [&]()
    {
        std::string page = R"html(
        <!DOCTYPE html>
        <html>
            <head>
                <script>
                    location.href = "https://github.com/Curve";
                </script>
            </head>
        </html>
        )html";

        webview.embed({{"index.html", saucer::embedded_file{
                                          .content = saucer::make_stash(page),
                                          .mime    = "text/html",
                                      }}});

        {
            auto guard = navigation_guard{webview};
            webview.serve("index.html");
        }

        expect(webview.url().find("github.com/Curve") != std::string::npos);

        webview.clear_embedded("index.html");

        {
            auto guard = navigation_guard{webview};
            webview.serve("index.html");
        }

        expect(webview.url().find("github.com/Curve") == std::string::npos);
    };

    "execute"_test = [&]()
    {
        {
            auto nav_guard = navigation_guard{webview};
            auto tit_guard = title_guard{webview};

            webview.set_url("https://saucer.github.io/");
        }

        {
            auto tit_guard = title_guard{webview};
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
            auto guard = navigation_guard{webview};
            webview.set_url("https://cppreference.com/");
        }

        webview.clear_scripts();
        expect(webview.url().find("isocpp.org") != std::string::npos);

        webview.inject("document.title = 'Hi!'", saucer::load_time::ready);

        {
            auto guard     = navigation_guard{webview};
            auto tit_guard = title_guard{webview};

            webview.set_url("https://cppreference.com/");
        }

        expect(webview.url().find("cppreference.com") != std::string::npos);
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
            auto guard = navigation_guard{webview};
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
