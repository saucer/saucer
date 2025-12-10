#include <saucer/smartview.hpp>

coco::stray start(saucer::application *app)
{
    auto window = saucer::window::create(app).value();

    auto first  = std::make_unique<saucer::smartview>(saucer::smartview::create({.window = window}).value());
    auto second = std::make_unique<saucer::smartview>(saucer::smartview::create({.window = window}).value());

    window->on<saucer::window::event::resize>(
        [&](auto width, auto height)
        {
            auto w = first && second ? width / 2 : width;
            auto o = first ? w : 0;
            auto h = height;

            if (first)
            {
                first->set_bounds({.x = 0, .y = 0, .w = w, .h = height});
            }

            if (!second)
            {
                return;
            }

            second->set_bounds({.x = o, .y = 0, .w = w, .h = height});
        });

    first->expose("remove", [&] { first.reset(); });
    second->expose("remove", [&] { second.reset(); });

    first->set_url("https://github.com/saucer/saucer");
    second->set_url("https://codeberg.org/saucer/saucer");

    first->set_dev_tools(true);
    second->set_dev_tools(true);

    window->show();

    co_await app->finish();
}

int main()
{
    return saucer::application::create({.id = "example"})->run(start);
}
