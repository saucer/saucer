#include "test.hpp"
#include "utils.hpp"

using namespace boost::ut;
using namespace saucer::tests;

suite<"window"> window_suite = []
{
    "visible"_test_both = [](auto &window)
    {
        window.show();
        expect(window.visible());

        window.hide();
        expect(not window.visible());
    };

    "resizable"_test_both = [](auto &window)
    {
        expect(window.resizable());

        window.set_resizable(false);
        expect(not window.resizable());
    };

#ifndef SAUCER_WEBKITGTK
    "minimize"_test_async = [](auto &window)
    {
        bool minimized{false};
        window.template on<saucer::window_event::minimize>([&minimized](bool value) { minimized = value; });

        window.set_minimized(true);
        saucer::tests::wait_for([&]() { return minimized; });

        expect(minimized);
        expect(window.minimized());

        window.set_minimized(false);
        saucer::tests::wait_for([&]() { return !minimized; });

        expect(not minimized);
        expect(not window.minimized());
    };
#endif

    "maximize"_test_async = [](auto &window)
    {
        bool maximized{false};
        window.template on<saucer::window_event::maximize>([&maximized](bool value) { maximized = value; });

        window.set_maximized(true);
        saucer::tests::wait_for([&]() { return maximized; });

        expect(maximized);
        expect(window.maximized());

        window.set_maximized(false);
        saucer::tests::wait_for([&]() { return !maximized; });

        expect(not maximized);
        expect(not window.maximized());
    };

    "title"_test_both = [](auto &window)
    {
        auto title = saucer::tests::random_string(10);

        window.set_title(title);
        expect(eq(window.title(), title));
    };

    "decoration"_test_both = [](auto &window)
    {
        using enum saucer::window_decoration;

        auto decoration = full;
        window.template on<saucer::window_event::decorated>([&decoration](auto value) { decoration = value; });

        expect(decoration == full);
        expect(window.decoration() == full);

        window.set_decoration(partial);

        expect(decoration == partial);
        expect(window.decoration() == partial);

        window.set_decoration(none);

        expect(decoration == none);
        expect(window.decoration() == none);
    };

    "size"_test_async = [](auto &window)
    {
        std::pair<int, int> size;
        window.template on<saucer::window_event::resize>([&size](int width, int height) { size = {width, height}; });

        window.set_size(400, 500);
        saucer::tests::wait_for([&] { return size == std::make_pair(400, 500); });

        expect(size == std::make_pair(400, 500));
        expect(window.size() == std::make_pair(400, 500));
    };

#ifndef SAUCER_WEBKITGTK
    "max_size"_test_both = [](auto &window)
    {
        window.set_max_size(200, 300);
        expect(window.max_size() == std::make_pair(200, 300));
    };
#endif

    "min_size"_test_both = [](auto &window)
    {
        window.set_min_size(200, 300);
        expect(window.min_size() == std::make_pair(200, 300));
    };
};
