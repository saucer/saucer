#include <print>

#include <saucer/smartview.hpp>
#include <saucer/modules/desktop.hpp>

int main()
{
    namespace modules = saucer::modules;
    using modules::picker::type;

    saucer::webview::register_scheme("test");

    auto app = saucer::application::acquire({
        .id = "example",
    });

    auto &desktop = app->add_module<modules::desktop>();

    saucer::smartview webview{{
        .application = app,
    }};

    webview.expose("pick_folder", [&] { return desktop.pick<type::folder>(); });
    webview.expose("pick_file", [&] { return desktop.pick<type::file>(); });

    webview.set_url("https://google.com");
    webview.set_dev_tools(true);

    webview.show();
    app->run();

    return 0;
}
