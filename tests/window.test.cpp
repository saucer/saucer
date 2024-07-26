#include "cfg.hpp"

#include <saucer/window.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

struct window : saucer::window
{
    window(const saucer::options &opts) : saucer::window(opts) {}
};

void tests(window &window)
{
    window.show();

    // Some tests have been disabled on QT6 due to upstream bugs which are yet to be fixed (minor wayland issues).

    bool was_minimized{false};
    window.on<saucer::window_event::minimize>([&](bool minimized) { was_minimized = minimized; });

    bool was_maximized{false};
    window.on<saucer::window_event::maximize>([&](bool maximized) { was_maximized = maximized; });

    "minimize"_test = [&]()
    {
        window.set_minimized(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(was_minimized);
        expect(window.minimized());

        window.set_minimized(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(not was_minimized);
        expect(not window.minimized());
    };

    "maximize"_test = [&]()
    {
        window.set_maximized(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(was_maximized);
        expect(window.maximized());

        window.set_maximized(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(not was_maximized);
        expect(not window.maximized());
    };

    "resizable"_test = [&]()
    {
        auto size = window.size();
        expect(window.resizable());

        window.set_resizable(false);
        expect(not window.resizable());

        window.set_size(size.first + 10, size.second + 10);

        auto [width, height] = window.size();
        expect(window.size() == size) << size.first << ":" << size.second << " -> " << width << ":" << height;
    };

#ifndef SAUCER_QT6
    "decorations"_test = [&]()
    {
        expect(window.decorations());

        window.set_decorations(false);
        expect(not window.decorations());

        window.set_decorations(true);
        expect(window.decorations());
    };

    "always_on_top"_test = [&]()
    {
        expect(not window.always_on_top());

        window.set_always_on_top(true);
        expect(window.always_on_top());

        window.set_always_on_top(false);
        expect(not window.always_on_top());
    };
#endif

    "title"_test = [&]()
    {
        window.set_title("Some Title");
        expect(window.title() == "Some Title") << window.title();
    };

    "size"_test = [&]()
    {
        window.set_resizable(true);
        window.set_size(500, 500);

        auto [width, height] = window.size();
        expect(width == 500 && height == 500) << width << ":" << height;
    };

    "max_size"_test = [&]()
    {
        window.set_max_size(600, 600);
        window.set_size(700, 700);

        auto [width, height] = window.size();
        expect(window.size() == std::make_pair(600, 600)) << width << ":" << height;
    };

    "min_size"_test = [&]()
    {
        window.set_min_size(200, 200);
        window.set_size(0, 0);

        auto [width, height] = window.size();
        expect(window.size() == std::make_pair(200, 200)) << width << ":" << height;
    };

    window.clear(saucer::window_event::maximize);
    window.clear(saucer::window_event::minimize);
}

suite<"window"> window_suite = []
{
    window window{{.hardware_acceleration = false}};

    "from-main"_test = [&]()
    {
        tests(window);
    };

    std::jthread thread{[&]()
                        {
                            std::this_thread::sleep_for(std::chrono::seconds(2));

                            "from-thread"_test = [&]()
                            {
                                tests(window);
                            };

                            window.close();
                        }};

    window.run();
};
