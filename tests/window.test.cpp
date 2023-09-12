#include <future>
#include <boost/ut.hpp>
#include <saucer/window.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

// NOLINTNEXTLINE
suite window_suite = []
{
    class dummy_window : public saucer::window
    {
    };

    "window"_test = [&]
    {
        auto window = dummy_window{};

        "resize"_test = [&]
        {
            window.set_resizable(false);
            expect(not window.resizable());

            window.set_resizable(true);
            expect(window.resizable());

            window.set_size(5, 5);
            expect(window.size() == std::make_pair(5, 5));

            window.set_max_size(50, 50);
            expect(window.max_size() == std::make_pair(50, 50));

            window.set_min_size(25, 25);
            expect(window.min_size() == std::make_pair(25, 25));
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
                if (width != 45)
                {
                    return;
                }

                expect(eq(width, 45));
                expect(eq(height, 45));

                window.clear(saucer::window_event::resize);
                window.close();
            };

            auto fut = std::make_shared<std::future<void>>();

            auto fn = [fut, &window]
            {
                std::this_thread::sleep_for(std::chrono::seconds(10));
                window.set_size(45, 45);
            };

            *fut = std::async(std::launch::async, std::move(fn));

            window.on<saucer::window_event::resize>(callback);
        };

        window.show();
        window.run();
    };
};