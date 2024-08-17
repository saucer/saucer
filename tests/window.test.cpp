#include "cfg.hpp"

#include <saucer/window.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

struct window : saucer::window
{
    window(const saucer::options &opts) : saucer::window(opts) {}
};

void tests(window &window, bool thread)
{
    window.show();

    bool was_minimized{false};
    window.on<saucer::window_event::minimize>([&](bool minimized) { was_minimized = minimized; });

    bool was_maximized{false};
    window.on<saucer::window_event::maximize>([&](bool maximized) { was_maximized = maximized; });

    std::pair<int, int> last_size{};
    window.on<saucer::window_event::resize>([&](int width, int height) { last_size = {width, height}; });

    "background"_test = [&]()
    {
        window.set_background({255, 0, 0, 255});

        auto [r, g, b, a] = window.background();
        expect(r == 255 && g == 0 && b == 0 && a == 255);
    };

#if !defined(SAUCER_WEBKITGTK) && !defined(SAUCER_WEBKIT)
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
#endif

    "maximize"_test = [&]()
    {
        window.set_maximized(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(!thread || was_maximized);
        expect(!thread || window.maximized());

        window.set_maximized(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(!thread || not was_maximized);
        expect(!thread || not window.maximized());
    };

    "resizable"_test = [&]()
    {
        auto size = window.size();
        expect(window.resizable());

        window.set_resizable(false);
        expect(not window.resizable());
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

#ifndef SAUCER_WEBKITGTK
    "always_on_top"_test = [&]()
    {
        expect(not window.always_on_top());

        window.set_always_on_top(true);
        expect(window.always_on_top());

        window.set_always_on_top(false);
        expect(not window.always_on_top());
    };
#endif
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

        {
            auto [width, height] = window.size();
            expect(width == 500 && height == 500) << width << ":" << height;
        }
#if !defined(SAUCER_QT5) && !defined(SAUCER_WEBKITGTK)
        {
            auto [width, height] = last_size;
            expect(width == 500 && height == 500) << width << ":" << height;
        }
#endif
    };

#ifndef SAUCER_WEBKITGTK
    "max_size"_test = [&]()
    {
        window.set_max_size(600, 600);
        expect(window.max_size() == std::make_pair(600, 600));
    };
#endif

    "min_size"_test = [&]()
    {
        window.set_min_size(200, 200);
        expect(window.min_size() == std::make_pair(200, 200));
    };

    window.clear(saucer::window_event::resize);
    window.clear(saucer::window_event::maximize);
    window.clear(saucer::window_event::minimize);
}

suite<"window"> window_suite = []
{
    window window{{.hardware_acceleration = false}};

    "from-main"_test = [&]()
    {
        tests(window, false);
    };

    const std::jthread thread{[&]()
                              {
                                  std::this_thread::sleep_for(std::chrono::seconds(2));

                                  "from-thread"_test = [&]()
                                  {
                                      tests(window, true);
                                  };

                                  window.close();
                              }};

    window.run();
};
