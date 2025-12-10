#include <saucer/smartview.hpp>
#include <saucer/modules/loop.hpp>

int main()
{
    auto app  = saucer::application::create({.id = "example"});
    auto loop = saucer::modules::loop{app.value()};

    auto window  = saucer::window::create(loop.application()).value();
    auto webview = saucer::smartview::create({.window = window});

    window->set_title("Hello World!");
    webview->set_url("https://github.com/saucer/saucer");

    window->show();
    loop.run();

    return 0;
}
