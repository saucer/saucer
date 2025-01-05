#include <saucer/webview.hpp>
#include <saucer/modules/pdf.hpp>

#include <iostream>
#include <print>

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::println(std::cerr, "Usage: {} <url> <file>", argv[0]);
        return 1;
    }

    auto app = saucer::application::init({
        .id = "pdf",
    });

    saucer::webview webview{{
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

            print.save({.file = file});
            webview.close();
        });

    webview.set_url(url);
    webview.show();

    app->run();

    return 0;
}
