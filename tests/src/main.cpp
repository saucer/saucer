#include "test.hpp"

int main()
{
    using namespace boost::ut;

    saucer::webview::register_scheme("test");

    auto application = saucer::application::create({
        .id                         = "tests",
        .quit_on_last_window_closed = false,
    });

    if (!application)
    {
        return 1;
    }

    int status{0};

    auto callback = [&](saucer::application *app) -> coco::stray
    {
        saucer::tests::g_application = app;
        status                       = co_await cfg<override>.run();
        app->quit();
    };

    return application->run(callback) | status;
}
