#include "test.hpp"
#include "utils.hpp"

using namespace boost::ut;
using namespace saucer::tests;

suite<"window"> window_suite = []
{
    using enum saucer::window::event;

    "visible"_test_both = [](saucer::window &window)
    {
        window.show();
        expect(window.visible());

        window.hide();
        expect(not window.visible());
    };

#ifndef SAUCER_WEBKITGTK
    "minimize"_test_async = [](saucer::window &window)
    {
        static constexpr auto duration = std::chrono::seconds(3);

        bool minimized{false};
        window.on<minimize>([&](bool value) { minimized = value; });

        window.set_minimized(true);
        saucer::tests::wait_for([&] { return minimized; }, duration);

        expect(minimized);
        expect(window.minimized());

        window.set_minimized(false);
        saucer::tests::wait_for([&] { return !minimized; }, duration);

        expect(not minimized);
        expect(not window.minimized());
    };
#endif

    "maximize"_test_async = [](saucer::window &window)
    {
        bool maximized{false};
        window.on<maximize>([&](bool value) { maximized = value; });

        window.set_maximized(true);
        saucer::tests::wait_for([&] { return maximized; });

        expect(maximized);
        expect(window.maximized());

        window.set_maximized(false);
        saucer::tests::wait_for([&] { return !maximized; });

        expect(not maximized);
        expect(not window.maximized());
    };

    "resizable"_test_both = [](saucer::window &window)
    {
        expect(window.resizable());

        window.set_resizable(false);
        expect(not window.resizable());

        window.set_resizable(true);
        expect(window.resizable());
    };

    "fullscreen"_test_async = [](saucer::window &window)
    {
        static constexpr auto duration = std::chrono::seconds(3);

        // We wait explicitly here, because macOS has a quite lengthy transition for fullscreening...

        expect(not window.fullscreen());

        window.set_fullscreen(true);
        std::this_thread::sleep_for(duration);

        expect(window.fullscreen());

        window.set_fullscreen(false);
        std::this_thread::sleep_for(duration);

        expect(not window.fullscreen());
    };

    "title"_test_both = [](saucer::window &window)
    {
        auto title = saucer::tests::random_string(10);

        window.set_title(title);
        expect(eq(window.title(), title));
    };

#ifndef SAUCER_WEBKITGTK
    "background"_test_both = [](saucer::window &window)
    {
        static constexpr auto red = saucer::color{.r = 255, .g = 0, .b = 0, .a = 255};

        expect(window.background() != red);

        window.set_background(red);
        expect(window.background() == red);
    };
#endif

    "decoration"_test_both = [](saucer::window &window)
    {
        using enum saucer::window::decoration;

        auto decoration = full;
        window.on<decorated>([&](auto value) { decoration = value; });

        expect(decoration == full);
        expect(window.decorations() == full);

#if !defined(SAUCER_QT)
        window.set_decorations(partial);

        expect(decoration == partial);
        expect(window.decorations() == partial);
#endif

        window.set_decorations(none);

        expect(decoration == none);
        expect(window.decorations() == none);
    };

    "size"_test_async = [](saucer::window &window)
    {
        saucer::size size{};
        window.on<resize>([&](int width, int height) { size = {.w = width, .h = height}; });

        window.set_size({.w = 400, .h = 500});
        saucer::tests::wait_for([&] { return size == saucer::size{.w = 400, .h = 500}; });

        expect(size == saucer::size{.w = 400, .h = 500});
        expect(window.size() == saucer::size{.w = 400, .h = 500});
    };

#ifndef SAUCER_WEBKITGTK
    "max_size"_test_both = [](saucer::window &window)
    {
        window.set_max_size({.w = 200, .h = 300});
        expect(window.max_size() == saucer::size{.w = 200, .h = 300});
    };
#endif

    "min_size"_test_both = [](saucer::window &window)
    {
        window.set_min_size({.w = 200, .h = 300});
        expect(window.min_size() == saucer::size{.w = 200, .h = 300});
    };

#ifndef SAUCER_WEBKITGTK
    "position"_test_both = [](saucer::window &window)
    {
        window.set_position({.x = 200, .y = 300});
        expect(window.position() == saucer::position{.x = 200, .y = 300});
    };
#endif
};
