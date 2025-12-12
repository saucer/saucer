#include <saucer/smartview.hpp>
#include <saucer/modules/pdf.hpp>

#include <filesystem>

static constexpr const char *demo = R"html(
<!DOCTYPE html>
<html>
    <body>
        <h1>Welcome to saucer!</h1>
        <p>This example demonstrates how to use the pdf module!</p>
        <hr>
        <p>The C++ code currently exposes the following functions:<p>
        <ul>
            <li>print(path: string)</li>
            <li>print_with(opts?)</li>
        </ul>
        <p>You can use the Dev-Tools to call them!</p>
        <p>See the "expose" example on how to interact with exposed functions</p>
    </body>
</html>
)html";

namespace fs = std::filesystem;

coco::stray start(saucer::application *app)
{
    auto window  = saucer::window::create(app).value();
    auto webview = saucer::smartview::create({.window = window});

    using enum saucer::modules::pdf::layout;
    auto pdf = saucer::modules::pdf{webview.value()};

    webview->expose("print",
                    [&](const fs::path &path)
                    {
                        pdf.save({
                            .file = path,
                            // A4 Page
                            // This has to be inches, as not all backends support the metric system (this is insane)
                            .size        = {.w = 8.27, .h = 11.69},
                            .orientation = portrait,
                        });
                    });

    webview->expose("print_with", [&](const saucer::modules::pdf::settings &settings) { pdf.save(settings); });

    webview->set_dev_tools(true);
    webview->set_html(demo);
    window->show();

    co_await app->finish();
}

int main()
{
    return saucer::application::create({.id = "example"})->run(start);
}
