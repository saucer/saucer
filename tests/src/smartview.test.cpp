#include "test.hpp"
#include "utils.hpp"

using namespace boost::ut;
using namespace saucer::tests;

struct some_struct
{
    int x;
};

suite<"smartview"> smartview_suite = []
{
    using string_vec = std::vector<std::string>;

    "evaluate"_test_async = [](const std::shared_ptr<saucer::smartview<>> &smartview)
    {
        smartview->set_url("https://saucer.github.io");

        expect(smartview->evaluate<int>("10 + 5").get() == 15);
        expect(smartview->evaluate<std::string>("'Hello' + ' ' + 'World'").get() == "Hello World");

        expect(smartview->evaluate<int>("{} + {}", 1, 2).get() == 3);
        expect(smartview->evaluate<std::string>("{} + {}", "C++", "23").get() == "C++23");
        expect(smartview->evaluate<string_vec>("Array.of({})", saucer::make_args("1", "2")).get() == string_vec{"1", "2"});
    };

    "expose-basic"_test_async = [](const std::shared_ptr<saucer::smartview<>> &smartview)
    {
        smartview->expose("sum", [](int a, int b) { //
            return a + b;
        });

        smartview->expose(
            "sub",
            [&](int a, int b) { //
                return smartview->evaluate<int>("{} - {}", a, b).get();
            },
            saucer::launch::async);

        smartview->expose("struct", [](const some_struct &data) { //
            return data.x;
        });

        smartview->set_url("https://saucer.github.io");

        expect(smartview->evaluate<int>("await saucer.exposed.sum(10, 5)").get() == 15);
        expect(smartview->evaluate<int>("await saucer.exposed.sub(10, 5)").get() == 5);

        expect(smartview->evaluate<int>("await saucer.exposed.struct({{ x: 5 }})").get() == 5);
        expect(smartview->evaluate<int>("await saucer.exposed.struct({})", some_struct{5}).get() == 5);
    };

    "expose-executor"_test_async = [](const std::shared_ptr<saucer::smartview<>> &smartview)
    {
        smartview->expose("sum", [](int a, int b, const saucer::executor<int> &exec) { //
            auto [resolve, reject] = exec;

            if (a < 0 || b < 0)
            {
                return reject("Not positive");
            }

            resolve(a + b);
        });

        std::optional<std::variant<int, std::string>> result;

        smartview->expose("resolve", [&](int res) { result = res; });
        smartview->expose("reject", [&](const std::string &reason) { result = reason; });

        smartview->set_url("https://saucer.github.io");

        smartview->evaluate<void>("saucer.exposed.sum(10, 5).then(res => saucer.exposed.resolve(res))").get();
        wait_for([&] { return result.has_value(); });

        expect(std::holds_alternative<int>(result.value()));
        expect(std::get<int>(result.value()) == 15);

        result.reset();

        smartview->evaluate<void>("saucer.exposed.sum(-10, -5).then(() => {{}}, res => saucer.exposed.reject(res))").get();
        wait_for([&] { return result.has_value(); });

        expect(std::holds_alternative<std::string>(result.value()));
        expect(std::get<std::string>(result.value()) == "Not positive");
    };
};
