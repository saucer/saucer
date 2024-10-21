#include "test.hpp"
#include "utils.hpp"

using namespace boost::ut;
using namespace saucer::tests;

suite<"window"> window_suite = []()
{
#ifndef SAUCER_WEBKITGTK
    "minimize"_test_async = [](const std::shared_ptr<saucer::smartview<>> &window)
    {
        bool minimized{false};
        window->on<saucer::window_event::minimize>([&minimized](bool value) { minimized = value; });

        window->set_minimized(true);

        wait_for(minimized);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(window->minimized());
        expect(minimized);

        window->set_minimized(false);

        wait_for([&] { return !minimized; });
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(not window->minimized());
        expect(not minimized);
    };
#endif

    "maximize"_test_async = [](const std::shared_ptr<saucer::smartview<>> &window)
    {
        bool maximized{false};
        window->on<saucer::window_event::maximize>([&maximized](bool value) { maximized = value; });

        window->set_maximized(true);

        wait_for(maximized);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(window->maximized());
        expect(maximized);

        window->set_maximized(false);

        wait_for([&] { return !maximized; });
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        expect(not window->maximized());
        expect(not maximized);
    };

    "resizable"_test_both = [](const auto &window)
    {
        expect(window->resizable());

        window->set_resizable(false);
        expect(not window->resizable());
    };

#ifndef SAUCER_QT6
    "decorations"_test_async = [](const std::shared_ptr<saucer::smartview<>> &window)
    {
        bool decorated{false};
        window->on<saucer::window_event::decorated>([&decorated](bool value) { decorated = value; });

        expect(window->decorations());

        window->set_decorations(false);
        wait_for([&] { return !decorated; });

        expect(not window->decorations());
        expect(not decorated);
    };

#ifndef SAUCER_WEBKITGTK
    "always_on_top"_test_both = [](const auto &window)
    {
        expect(not window->always_on_top());

        window->set_always_on_top(true);
        expect(window->always_on_top());
    };
#endif
#endif

    "title"_test_both = [](const auto &window)
    {
        window->set_title("Some Title");
        expect(window->title() == "Some Title") << window->title();
    };

    "size"_test_async = [](const std::shared_ptr<saucer::smartview<>> &window)
    {
        std::pair<int, int> size;
        window->on<saucer::window_event::resize>([&size](int width, int height) { size = {width, height}; });

        window->set_size(400, 500);
        wait_for([&] { return size.first == 400 && size.second == 500; });

        auto [width, height] = window->size();
        expect(width == 400 && height == 500) << width << ":" << height;

        auto [last_width, last_height] = size;
        expect(width == last_width && height == last_height) << last_width << ":" << last_height;
    };

#ifndef SAUCER_WEBKITGTK
    "max_size"_test_both = [](const auto &window)
    {
        window->set_max_size(200, 300);

        auto [width, height] = window->max_size();
        expect(width == 200 && height == 300) << width << ":" << height;
    };
#endif

    "min_size"_test_both = [](const auto &window)
    {
        window->set_min_size(200, 300);

        auto [width, height] = window->min_size();
        expect(width == 200 && height == 300) << width << ":" << height;
    };
};
