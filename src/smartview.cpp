#include "smartview.hpp"

#include "webview.impl.hpp"

#include "scripts.hpp"
#include "webview.hpp"

#include <atomic>

#include <lockpp/lock.hpp>

namespace saucer
{
    using lockpp::lock;

    using resolver = serializer_core::resolver;
    using function = serializer_core::function;

    struct smartview_core::impl
    {
        using exposed = std::shared_ptr<function>;

      public:
        std::shared_ptr<lockpp::lock<webview::impl *>> webview;

      public:
        std::atomic_uint64_t id_counter{0};
        std::unique_ptr<serializer_core> serializer;

      public:
        lock<std::unordered_map<std::string, exposed>> functions;
        lock<std::unordered_map<std::uint64_t, resolver>> evaluations;

      public:
        void call(std::unique_ptr<function_data>);
        void resolve(std::unique_ptr<result_data>);
    };

    smartview_core::smartview_core(webview &&base, std::unique_ptr<serializer_core> serializer)
        : webview(std::move(base)), m_impl(std::make_unique<impl>())
    {
        using namespace scripts;

        m_impl->webview    = std::make_shared<lockpp::lock<webview::impl *>>(webview::m_impl.get());
        m_impl->serializer = std::move(serializer);

        auto script = std::format(smartview_script, m_impl->serializer->js_serializer());

        inject({.code = std::move(script), .time = load_time::creation, .permanent = true});
        inject({.code = m_impl->serializer->script(), .time = load_time::creation, .permanent = true});

        auto on_message = [impl = m_impl.get()](std::string_view message)
        {
            auto parsed = impl->serializer->parse(message);

            overload visitor = {
                [](std::monostate &) { return false; },
                [impl](std::unique_ptr<function_data> &parsed)
                {
                    impl->call(std::move(parsed));
                    return true;
                },
                [impl](std::unique_ptr<result_data> &parsed)
                {
                    impl->resolve(std::move(parsed));
                    return true;
                },
            };

            return std::visit(visitor, parsed);
        };

        on<webview::event::message>(std::move(on_message));
    }

    smartview_core::smartview_core(smartview_core &&) noexcept = default;

    smartview_core::~smartview_core()
    {
        if (!m_impl)
        {
            return;
        }

        m_impl->webview->assign(nullptr);
    }

    void smartview_core::impl::call(std::unique_ptr<function_data> message)
    {
        exposed function;

        if (auto locked = functions.write(); locked->contains(message->name))
        {
            function = locked->at(message->name);
        }
        else
        {
            return webview->get_unsafe()->reject(message->id, std::format("\"No exposed function '{}'\"", message->name));
        }

        auto resolve = [shared = webview, id = message->id](const auto &result)
        {
            auto webview = shared->read();

            if (!webview.value())
            {
                return;
            }

            webview.value()->resolve(id, result);
        };

        auto reject = [shared = webview, id = message->id](const auto &error)
        {
            auto webview = shared->read();

            if (!webview.value())
            {
                return;
            }

            webview.value()->reject(id, error);
        };

        auto executor = serializer_core::executor{
            std::move(resolve),
            std::move(reject),
        };

        return std::invoke(*function, std::move(message), std::move(executor));
    }

    void smartview_core::impl::resolve(std::unique_ptr<result_data> message)
    {
        resolver evaluation;

        if (auto locked = evaluations.write(); auto node = locked->extract(message->id))
        {
            evaluation = std::move(node.mapped());
        }
        else
        {
            return;
        }

        std::invoke(evaluation, std::move(message));
    }

    void smartview_core::add_function(std::string name, function &&resolve)
    {
        auto functions = m_impl->functions.write();
        functions->emplace(std::move(name), std::make_shared<function>(std::move(resolve)));
    }

    void smartview_core::add_evaluation(resolver &&resolve, std::string_view code)
    {
        auto id = m_impl->id_counter++;

        {
            auto locked = m_impl->evaluations.write();
            locked->emplace(id, std::move(resolve));
        }

        webview::execute(std::format(
            R"(
                (async () =>
                    window.saucer.internal.resolve({}, {})
                )();
            )",
            id, code));
    }

    void smartview_core::unexpose()
    {
        auto locked = m_impl->functions.write();
        locked->clear();
    }

    void smartview_core::unexpose(const std::string &name)
    {
        auto locked = m_impl->functions.write();
        locked->erase(name);
    }
} // namespace saucer
