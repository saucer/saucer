#include "cfg.hpp"

#include <future>
#include <saucer/window.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

suite window_suite = []
{
    struct dummy_window : public saucer::window
    {
        dummy_window(const saucer::options &opts) : saucer::window(opts) {}
    };

    "window"_test = [&]
    {
        dummy_window window({.hardware_acceleration = false});

        "resize"_test = [&]
        {
            window.set_resizable(false);
            expect(not window.resizable());

            window.set_resizable(true);
            expect(window.resizable());

            window.set_min_size(250, 250);
            expect(window.min_size() == std::make_pair(250, 250));

            window.set_size(300, 300);
            expect(window.size() == std::make_pair(300, 300));

            window.set_max_size(500, 500);
            expect(window.max_size() == std::make_pair(500, 500));
        };

        "decorations"_test = [&]
        {
            window.set_decorations(false);
            expect(not window.decorations());

            window.set_decorations(true);
            expect(window.decorations());
        };

        "title"_test = [&]
        {
            window.set_title("Test!");
            expect(window.title() == "Test!");

            window.set_title("Заголовок!");
            expect(window.title() == "Заголовок!");

            window.set_title("標題!");
            expect(window.title() == "標題!");
        };

        "always_on_top"_test = [&]
        {
            window.set_always_on_top(true);
            expect(window.always_on_top());

            window.set_always_on_top(false);
            expect(not window.always_on_top());
        };

        "events"_test = [&]
        {
            auto callback = [&](int width, int height)
            {
                if (width != 450)
                {
                    return;
                }

                expect(eq(width, 450));
                expect(eq(height, 450));

                window.clear(saucer::window_event::resize);
                window.close();
            };

            auto fut = std::make_shared<std::future<void>>();

            auto fn = [fut, &window]
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                window.set_size(450, 450);
            };

            *fut = std::async(std::launch::async, std::move(fn));

            window.on<saucer::window_event::resize>(callback);
        };

        window.show();
        window.run();
    };
};
