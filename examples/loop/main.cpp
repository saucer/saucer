#include <saucer/smartview.hpp>
#include <saucer/modules/loop.hpp>

int main()
{
    auto app   = saucer::application::create({.id = "example"});
    auto &loop = app->add_module<saucer::modules::loop>();

    auto webview = saucer::smartview{{
        .application = loop.application(),
    }};

    webview.set_title("Hello World!");
    webview.set_size(500, 600);

    webview.set_min_size(400, 500);
    webview.set_max_size(1000, 1200);

    // Set the URL, Show the Dev-Tools and run

    webview.set_url("https://github.com/saucer/saucer");
    webview.set_dev_tools(true);

    webview.show();

    loop.run();

    return 0;
}
