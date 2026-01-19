#include "test.hpp"
#include "utils.hpp"

using namespace boost::ut;
using namespace saucer::tests;

suite<"webview"> webview_suite = []
{
    using enum saucer::webview::event;

    static constexpr auto duration = std::chrono::seconds(10);

    "url"_test_async = [](saucer::webview &webview)
    {
        auto url = webview.url();
        webview.on<navigated>([&](auto value) { url = std::move(value); });

        std::set<saucer::state> state;
        webview.on<load>([&](auto value) { state.emplace(value); });

        bool ready{false};
        webview.on<dom_ready>([&] { ready = true; });

        auto parsed = saucer::url::parse("https://codeberg.org/saucer/saucer");
        expect(parsed.has_value());

        webview.set_url(parsed.value());
        saucer::tests::wait_for([&] { return url != saucer::url{} && ready && state.contains(saucer::state::finished); }, duration);

        expect(ready);
        expect(url != saucer::url{});

        expect(state.contains(saucer::state::started));
        expect(state.contains(saucer::state::finished));

        expect(url.scheme() == "https");
        expect(url.host() == "codeberg.org");
        expect(url.path() == "/saucer/saucer");
    };

    "page_title"_test_async = [](saucer::webview &webview)
    {
        std::string page_title{};
        webview.on<title>([&](auto value) { page_title = value; });

        webview.set_url("https://codeberg.org/saucer/saucer");
        saucer::tests::wait_for([&] { return page_title.contains("saucer"); }, duration);

        expect(page_title.starts_with("saucer/saucer"));
        expect(webview.page_title().starts_with("saucer/saucer"));
    };

    "dev_tools"_test_both = [](saucer::webview &webview)
    {
        expect(not webview.dev_tools());

        webview.set_dev_tools(true);
        expect(webview.dev_tools());

        webview.set_dev_tools(false);
        expect(not webview.dev_tools());
    };

    "context-menu"_test_both = [](saucer::webview &webview)
    {
        expect(webview.context_menu());

        webview.set_context_menu(false);
        expect(not webview.context_menu());

        webview.set_context_menu(true);
        expect(webview.context_menu());
    };

    "force-dark"_test_both = [](saucer::webview &webview)
    {
        expect(not webview.force_dark());

        webview.set_force_dark(true);
        expect(webview.force_dark());

        webview.set_force_dark(false);
        expect(not webview.force_dark());
    };

    "background"_test_both = [](saucer::webview &webview)
    {
        static constexpr auto green = saucer::color{.r = 0, .g = 255, .b = 0, .a = 255};

        expect(webview.background() != green);

        webview.set_background(green);
        expect(webview.background() == green);
    };

    "execute"_test_async = [](saucer::webview &webview)
    {
        auto url = webview.url();
        webview.on<navigated>([&](auto value) { url = std::move(value); });

        webview.set_url("https://github.com");
        webview.execute("location.href = 'https://codeberg.org'");

        saucer::tests::wait_for([&] { return url != saucer::url{} && url.host() == "codeberg.org"; }, duration);

        expect(url != saucer::url{});
        expect(url.host() == "codeberg.org");
        expect(webview.url().host() == "codeberg.org");
    };

    "inject"_test_async = [](saucer::webview &webview)
    {
        std::set<std::string> messages;
        webview.on<message>(
            [&](auto value)
            {
                messages.emplace(std::move(value));
                return saucer::status::unhandled;
            });

        webview.inject({
            .code   = "saucer.internal.message('creation')",
            .run_at = saucer::script::time::creation,
        });

        webview.inject({
            .code   = "saucer.internal.message('ready')",
            .run_at = saucer::script::time::ready,
        });

        const auto id = webview.inject({
            .code      = "saucer.internal.message('permanent')",
            .run_at    = saucer::script::time::ready,
            .clearable = false,
        });

        webview.set_url("https://codeberg.org/saucer/saucer");
        saucer::tests::wait_for([&] { return messages.contains("ready"); }, duration);

        expect(messages.contains("creation"));
        expect(messages.contains("ready"));
        expect(messages.contains("permanent"));

        messages.clear();
        webview.uninject();

        webview.reload();
        saucer::tests::wait_for([&] { return messages.contains("ready"); }, duration);

        expect(not messages.contains("creation"));
        expect(not messages.contains("ready"));
        expect(messages.contains("permanent"));

        messages.clear();
        webview.uninject(id);

        webview.reload();
        saucer::tests::wait_for([&] { return messages.contains("ready"); }, duration);

        expect(not messages.contains("permanent"));
    };

    "embed"_test_async = [](saucer::webview &webview)
    {
        static constexpr auto duration  = std::chrono::seconds(3);
        static constexpr auto handle_if = [](auto &value, bool cond)
        {
            if (!cond)
            {
                return saucer::status::unhandled;
            }

            value = true;
            return saucer::status::handled;
        };

        bool embedded{false};
        webview.on<message>([&](auto value) { return handle_if(embedded, value == "embedded"); });

        static constexpr std::string_view page = R"html(
                <!DOCTYPE html>
                <html>
                    <head>
                        <title>Embedded</title>
                        <script>
                            saucer.internal.message("embedded");
                        </script>
                    </head>
                    <body>
                        Embedded Test
                    </body>
                </html>
            )html";

        std::size_t called{0};
        auto lazy = [&]()
        {
            called++;
            return saucer::stash::view_str(page);
        };

        webview.embed({{"/embed.html", saucer::embedded_file{
                                           .content = saucer::stash::lazy(lazy),
                                           .mime    = "text/html",
                                       }}});

        webview.serve("/embed.html");
        saucer::tests::wait_for([&] { return embedded; }, duration);

        expect(embedded);
        expect(called == 1);

        embedded = false;

        webview.reload();
        saucer::tests::wait_for([&] { return embedded; }, duration);

        expect(embedded);
        expect(called == 1);

        embedded = false;
        webview.unembed("/embed.html");

        webview.reload();
        saucer::tests::wait_for([&] { return embedded; }, duration);

        expect(not embedded);
    };

    "scheme"_test_async = [](saucer::webview &webview)
    {
        static constexpr auto duration  = std::chrono::seconds(3);
        static constexpr auto handle_if = [](auto &value, bool cond)
        {
            if (!cond)
            {
                return saucer::status::unhandled;
            }

            value = true;
            return saucer::status::handled;
        };

        bool scheme{false};
        webview.on<message>([&](auto value) { return handle_if(scheme, value == "scheme"); });

        static constexpr std::string_view page = R"html(
                <!DOCTYPE html>
                <html>
                    <head>
                        <title>Scheme</title>
                        <script>
                            saucer.internal.message("scheme");
                        </script>
                    </head>
                    <body>
                        Scheme Test
                    </body>
                </html>
            )html";

        webview.handle_scheme("test",
                              [](const saucer::scheme::request &req)
                              {
                                  expect(req.method() == "GET");
                                  expect(req.url().scheme() == "test");

                                  expect(req.url().host() == "host");
                                  expect(req.url().path() == "/test.html");

                                  return saucer::scheme::response{
                                      .data   = saucer::stash::view_str(page),
                                      .mime   = "text/html",
                                      .status = 200,
                                  };
                              });

        webview.set_url(saucer::url::make({.scheme = "test", .host = "host", .path = "/test.html"}));
        saucer::tests::wait_for([&] { return scheme; }, duration);

        expect(scheme);

        scheme = false;
        webview.remove_scheme("test");

        webview.reload();
        saucer::tests::wait_for([&] { return scheme; }, duration);

        expect(not scheme);
    };
};
