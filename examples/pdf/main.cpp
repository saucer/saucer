#include <print>

#include <saucer/webview.hpp>
#include <saucer/modules/pdf.hpp>

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::println(stderr, "Usage: {} <url> <file>", argv[0]);
        return 1;
    }

    auto start = [argc, argv](saucer::application *app) -> coco::stray
    {
        auto webview = saucer::webview{{
            .application = app,
        }};

        auto &print = webview.add_module<saucer::modules::pdf>();

        auto *url  = argv[1];
        auto *file = argv[2];

        webview.on<saucer::web_event::load>(
            [&](saucer::state state)
            {
                if (state != saucer::state::finished)
                {
                    return;
                }

                print.save({.file = file, .size = {27.2, 15.3}});
                webview.close();
            });

        webview.set_url(url);
        webview.show();

        co_await app->finish();
    };

    return saucer::application::create({.id = "pdf"})->run(start);
}
