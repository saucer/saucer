#define CONFIG_CATCH_MAIN
#include <saucer/window.hpp>
#include <catch2/catch.hpp>
#include <future>
#include <thread>

class test_window : public saucer::window
{
};

TEST_CASE("Window functionality is tested", "[window]")
{
    auto window = test_window();

    window.set_resizable(false);
    REQUIRE_FALSE(window.resizable());
    window.set_resizable(true);
    REQUIRE(window.resizable());

    window.set_decorations(true);
    REQUIRE(window.decorations());
    window.set_decorations(false);
    REQUIRE(!window.decorations());

    window.set_background({255, 0, 0, 255});
    REQUIRE(window.background() == saucer::color{255, 0, 0, 255});

    window.set_title("Hello World!");
    REQUIRE(window.title() == "Hello World!");

    window.set_max_size(100, 100);
    REQUIRE(window.max_size() == std::pair<int, int>{100, 100});

    window.set_min_size(50, 50);
    REQUIRE(window.min_size() == std::pair<int, int>{50, 50});

    window.set_size(70, 70);
    REQUIRE(window.size() == std::pair<int, int>{70, 70});

    //! Hide & Show require platform specifc code.
    //! On Close requires user input

    window.on<saucer::window_event::resize>([&](auto width, auto height) {
        static std::once_flag flag;

        std::call_once(flag, [&] {
            REQUIRE(width == 70);
            REQUIRE(height == 70);

            auto fut_ptr = std::make_shared<std::future<void>>();
            *fut_ptr = std::async(std::launch::async, [fut_ptr, &window] {
                //? Now we'll test thread safe functions
                std::this_thread::sleep_for(std::chrono::seconds(2));

                window.set_size(60, 60);
                REQUIRE(window.size() == std::pair<int, int>{60, 60});

                window.set_max_size(200, 200);
                REQUIRE(window.max_size() == std::pair<int, int>{200, 200});

                window.set_min_size(0, 0);
                REQUIRE(window.min_size() == std::pair<int, int>{0, 0});

                window.set_decorations(true);
                REQUIRE(window.decorations());
                window.set_decorations(false);
                REQUIRE(!window.decorations());

                window.set_always_on_top(true);
                REQUIRE(window.always_on_top());
                window.set_always_on_top(false);
                REQUIRE_FALSE(window.always_on_top());

                window.close();
            });
        });

        if (width == 60 && height == 60)
        {
            window.set_always_on_top(true);
            REQUIRE(window.always_on_top());
            window.set_always_on_top(false);
            REQUIRE_FALSE(window.always_on_top());
        }
    });

    window.show();
    window.run();
}