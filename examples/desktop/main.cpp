#include <saucer/smartview.hpp>
#include <saucer/modules/desktop.hpp>

static constexpr const char *demo = R"html(
<!DOCTYPE html>
<html>
    <body>
        <h1>Welcome to saucer!</h1>
        <p>This example demonstrates how to use the desktop module!</p>
        <hr>
        <p>The C++ code currently exposes the following functions:<p>
        <ul>
            <li>pick_folder(opts?)</li>
            <li>pick_file(opts?)</li>
            <li>open(url: string)</li>
            <li>mouse_position()</li>
        </ul>
        <p>You can use the Dev-Tools to call them!</p>
        <p>See the "expose" example on how to interact with exposed functions</p>
    </body>
</html>
)html";

coco::stray start(saucer::application *app)
{
    using saucer::modules::picker::type;

    auto desktop = saucer::modules::desktop{app};

    auto window  = saucer::window::create(app).value();
    auto webview = saucer::smartview::create({.window = window});

    webview->expose("pick_folder", [&](saucer::modules::picker::options opts) { return desktop.pick<type::folder>(std::move(opts)); });
    webview->expose("pick_file", [&](saucer::modules::picker::options opts) { return desktop.pick<type::file>(std::move(opts)); });

    webview->expose("open", [&](const std::string &url) { desktop.open(url); });
    webview->expose("mouse_position", [&] { return desktop.mouse_position(); });

    webview->set_dev_tools(true);
    webview->set_html(demo);
    window->show();

    co_await app->finish();
}

int main()
{
    return saucer::application::create({.id = "example"})->run(start);
}
