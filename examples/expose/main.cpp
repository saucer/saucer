#include <saucer/smartview.hpp>

static constexpr std::string_view demo = R"html(
<!DOCTYPE html>
<html>
    <body>
        <h1>Welcome to saucer!</h1>
        <p>This example demonstrates how to use the JS <-> C++ Brdige</p>
        <hr>
        <p>The C++ code currently exposes the following functions:<p>
        <ul>
            <li>add(number, number)</li>
            <li>add_random(number, number)</li>
            <li>add_object({ a: number, b: number })</li>
            <li>divide(number, number)</li>
            <li>divide_slow(number, number)</li>
            <li>divide_random(number, number)</li>
        </ul>
        <p>You can use the Dev-Tools to call them!</p>
        <p>The following two variants of function invocation exist:</p>
        <ul>
            <li>await saucer.call("add", [10, 5]);</li>
            <li>await saucer.exposed.add(10, 5);</li>
        </ul>
        <p>The function arguments will automatically be type-checked!</p>
    </body>
</html>
)html";

struct simple_aggregate
{
    double a;
    double b;
};

coco::stray start(saucer::application *app)
{
    auto window  = saucer::window::create(app).value();
    auto webview = saucer::smartview<>::create({.window = window});

    webview->expose("add",
                    [](double a, double b)
                    {
                        // A simple exposed function. Takes two doubles, returns a double.
                        return a + b;
                    });

    webview->expose("add_random",
                    [&](double a, double b) -> coco::task<double>
                    {
                        // We can also use `evaluate` to evaluate JavaScript code and capture its result:
                        auto random = co_await webview->evaluate<double>("Math.random()");
                        co_return a + b + random;
                    });

    webview->expose("add_object",
                    [](const simple_aggregate &aggregate)
                    {
                        // Arbitrary types? No problem. See the docs for usage with more complex types.
                        return aggregate.a + aggregate.b;
                    });

    webview->expose("divide",
                    [](double a, double b) -> std::expected<double, std::string>
                    {
                        if (b == 0)
                        {
                            // We can use `std::unexpected` to reject the promise
                            return std::unexpected{"Second operand must not be zero"};
                        }

                        return a / b;
                    });

    webview->expose("divide_slow",
                    [](double a, double b, saucer::executor<double> executor) mutable
                    {
                        // When taking a `saucer::executor` as the last argument, you have more control over when to resolve/reject the
                        // promise. We can pass the executor to a thread and resolve it sometime later.

                        std::thread t(
                            [a, b, executor = std::move(executor)]
                            {
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                                const auto &[resolve, reject] = executor;

                                if (b == 0)
                                {
                                    return reject("Second operand must not be zero");
                                }

                                return resolve(a / b);
                            });

                        t.detach();
                    });

    webview->expose("divide_random",
                    [&](double a, double b) -> coco::task<std::expected<double, std::string>>
                    {
                        // Remember how we used `std::expected` earlier to reject the promise?
                        // Works the same way for a coroutine that returns `std::expected`!

                        if (b == 0)
                        {
                            co_return std::unexpected{"Second operand must not be zero"};
                        }

                        co_return (a / b) / co_await webview->evaluate<double>("Math.random()");
                    });

    webview->embed({{"/index.html", {.content = saucer::stash<>::view(demo), .mime = "text/html"}}});

    webview->set_dev_tools(true);
    webview->serve("/index.html");

    window->show();

    co_await app->finish();
}

int main()
{
    return saucer::application::create({.id = "example"})->run(start);
}
