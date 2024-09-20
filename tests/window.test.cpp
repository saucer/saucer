#include "cfg.hpp"

#include <saucer/window.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

struct window : saucer::window
{
    window(const saucer::preferences &prefs) : saucer::window(prefs) {}
};

static void tests(window &window, bool thread)
{
    window.show();

    bool was_decorated{false};
    window.on<saucer::window_event::decorated>([&](bool decorated) { was_decorated = decorated; });

    bool was_maximized{false};
    window.on<saucer::window_event::maximize>([&](bool maximized) { was_maximized = maximized; });

    bool was_minimized{false};
    window.on<saucer::window_event::minimize>([&](bool minimized) { was_minimized = minimized; });

    std::pair<int, int> last_size;
    window.on<saucer::window_event::resize>([&](int width, int height) { last_size = {width, height}; });

#if !defined(SAUCER_WEBKITGTK) && !defined(SAUCER_WEBKIT)
    "minimize"_test = [&]()
    {
        window.set_minimized(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(window.minimized());
        expect(!thread || was_minimized);

        window.set_minimized(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(not window.minimized());
        expect(!thread || not was_minimized);
    };
#endif

    "maximize"_test = [&]()
    {
        window.set_maximized(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(!thread || window.maximized());
        expect(!thread || was_maximized);

        window.set_maximized(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(!thread || not window.maximized());
        expect(!thread || not was_maximized);
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
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(not window.decorations());
        expect(!thread || not was_decorated);

        window.set_decorations(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(window.decorations());
        expect(!thread || was_decorated);
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

#ifndef SAUCER_QT5
    "size"_test = [&]()
    {
        window.set_resizable(true);

        window.set_size(500, 500);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        auto [width, height] = window.size();
        expect(width == 500 && height == 500) << width << ":" << height;

        auto [last_width, last_height] = last_size;
        expect(!thread || (width == last_width && height == last_height)) << last_width << ":" << last_height;
    };
#endif

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
    window.clear(saucer::window_event::decorated);

    window.close();
}

suite<"window"> window_suite = []
{
    window first{{.hardware_acceleration = false}};
    window second{{.hardware_acceleration = false}};

    const std::jthread thread{[&]()
                              {
                                  "from-thread"_test = [&second]()
                                  {
                                      tests(second, true);
                                  };
                              }};

    "from-main"_test = [&first]()
    {
        tests(first, false);
    };

    saucer::window::run();
};
