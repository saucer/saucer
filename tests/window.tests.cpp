#define CONFIG_CATCH_MAIN
#include <catch2/catch.hpp>
#include <window.hpp>
#include <future>
#include <thread>

class test_window : public saucer::window
{
};

TEST_CASE("Window functionality is tested", "[window]")
{
    auto window = test_window();

    window.set_resizeable(false);
    REQUIRE_FALSE(window.get_resizeable());
    window.set_resizeable(true);
    REQUIRE(window.get_resizeable());

    window.set_decorations(true);
    REQUIRE(window.get_decorations());
    window.set_decorations(false);
    REQUIRE(!window.get_decorations());

    window.set_background_color(255, 0, 0, 255);
    REQUIRE(window.get_background_color() == std::make_tuple(255, 0, 0, 255));

    window.set_title("Hello World!");
    REQUIRE(window.get_title() == "Hello World!");

    window.set_max_size(100, 100);
    REQUIRE(window.get_max_size() == std::pair<std::size_t, std::size_t>(100, 100));

    window.set_min_size(50, 50);
    REQUIRE(window.get_min_size() == std::pair<std::size_t, std::size_t>(50, 50));

    window.set_size(70, 70);
    REQUIRE(window.get_size() == std::pair<std::size_t, std::size_t>(70, 70));

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
                REQUIRE(window.get_size() == std::pair<std::size_t, std::size_t>(60, 60));

                window.set_max_size(200, 200);
                REQUIRE(window.get_max_size() == std::pair<std::size_t, std::size_t>(200, 200));

                window.set_min_size(0, 0);
                REQUIRE(window.get_min_size() == std::pair<std::size_t, std::size_t>(0, 0));

                window.set_decorations(true);
                REQUIRE(window.get_decorations());
                window.set_decorations(false);
                REQUIRE(!window.get_decorations());

                window.set_always_on_top(true);
                REQUIRE(window.get_always_on_top());
                window.set_always_on_top(false);
                REQUIRE_FALSE(window.get_always_on_top());

                window.close();
            });
        });

        if (width == 60 && height == 60)
        {
            window.set_always_on_top(true);
            REQUIRE(window.get_always_on_top());
            window.set_always_on_top(false);
            REQUIRE_FALSE(window.get_always_on_top());
        }
    });

    window.show();
    window.run();
}