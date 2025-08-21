#include <saucer/smartview.hpp>
#include <saucer/modules/desktop.hpp>

static constexpr std::string_view demo = R"html(
<!DOCTYPE html>
<html>
    <body>
        <h1>Welcome to saucer!</h1>
        <p>This example demonstrates how to use the desktop module!</p>
        <hr>
        <p>The C++ code currently exposes the following functions:<p>
        <ul>
            <li>pick_folder(initial_dir?: string)</li>
            <li>pick_file(filters?: string[])</li>
            <li>open(uri: string)</li>
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
    auto webview = saucer::smartview<>::create({.window = window});

    webview->expose("pick_folder",
                    [&](std::optional<std::string> initial) { return desktop.pick<type::folder>({.initial = std::move(initial)}); });

    webview->expose("pick_file",
                    [&](std::set<std::string> filters) { return desktop.pick<type::file>({.filters = std::move(filters)}); });

    webview->expose("open", [&](const std::string &uri) { desktop.open(uri); });
    webview->expose("mouse_position", [&] { return desktop.mouse_position(); });

    webview->embed({{"/index.html", saucer::embedded_file{.content = saucer::stash<>::view(demo), .mime = "text/html"}}});
    webview->serve("/index.html");

    webview->set_dev_tools(true);
    window->show();

    co_await app->finish();
}

int main()
{
    return saucer::application::create({.id = "example"})->run(start);
}
