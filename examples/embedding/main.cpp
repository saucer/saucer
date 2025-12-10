#include <saucer/smartview.hpp>
#include <saucer/embedded/all.hpp>

coco::stray start(saucer::application *app)
{
    auto window  = saucer::window::create(app).value();
    auto webview = saucer::smartview::create({.window = window});

    webview->embed(saucer::embedded::all());
    webview->set_dev_tools(true);

    webview->serve("/src/index.html");
    window->show();

    co_await app->finish();
}

int main()
{
    return saucer::application::create({.id = "embedding"})->run(start);
}
