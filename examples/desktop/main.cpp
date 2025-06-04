#include <saucer/smartview.hpp>
#include <saucer/modules/desktop.hpp>

coco::stray start(saucer::application *app)
{
    using saucer::modules::picker::type;

    auto &desktop = app->add_module<saucer::modules::desktop>();

    auto webview = saucer::smartview{{
        .application = app,
    }};

    webview.expose("pick_folder", [&] { return desktop.pick<type::folder>(); });
    webview.expose("pick_file", [&] { return desktop.pick<type::file>(); });

    webview.set_url("https://google.com");
    webview.set_dev_tools(true);

    webview.show();

    co_await app->finish();
}

int main()
{
    return saucer::application::create({.id = "example"})->run(start);
}
