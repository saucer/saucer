#include "smartview.hpp"

#include "scripts.hpp"

#include <lockpp/lock.hpp>
#include <fmt/core.h>

namespace saucer
{
    using lockpp::lock;

    using resolver = saucer::serializer::resolver;
    using function = saucer::serializer::function;

    struct smartview_core::impl
    {
        using exposed = std::shared_ptr<function>;

      public:
        lock<std::unordered_map<std::string, exposed>> functions;
        lock<std::unordered_map<std::uint64_t, resolver>> evaluations;

      public:
        std::unique_ptr<saucer::serializer> serializer;
        std::shared_ptr<lockpp::lock<smartview_core *>> self;
    };

    smartview_core::smartview_core(std::unique_ptr<serializer> serializer, const preferences &prefs)
        : webview(prefs), m_impl(std::make_unique<impl>())
    {
        using namespace scripts;

        m_impl->serializer = std::move(serializer);
        m_impl->self       = std::make_shared<lockpp::lock<smartview_core *>>(this);

        auto script = fmt::format(smartview_script, fmt::arg("serializer", m_impl->serializer->js_serializer()));

        inject({.code = std::move(script), .time = load_time::creation, .permanent = true});
        inject({.code = m_impl->serializer->script(), .time = load_time::creation, .permanent = true});
    }

    smartview_core::~smartview_core()
    {
        auto locked = m_impl->self->write();
        *locked     = nullptr;
    }

    bool smartview_core::on_message(const std::string &message)
    {
        if (webview::on_message(message))
        {
            return true;
        }

        auto parsed = m_impl->serializer->parse(message);

        overload visitor = {
            [](std::monostate &) { return false; },
            [this](std::unique_ptr<function_data> &parsed)
            {
                call(std::move(parsed));
                return true;
            },
            [this](std::unique_ptr<result_data> &parsed)
            {
                resolve(std::move(parsed));
                return true;
            },
        };

        return std::visit(visitor, parsed);
    }

    void smartview_core::call(std::unique_ptr<function_data> message)
    {
        impl::exposed exposed;

        if (auto locked = m_impl->functions.write(); locked->contains(message->name))
        {
            exposed = locked->at(message->name);
        }
        else
        {
            return reject(message->id, fmt::format("\"No exposed function '{}'\"", message->name));
        }

        auto resolve = [shared = m_impl->self, id = message->id](const auto &result)
        {
            auto self = shared->read();

            if (!self.value())
            {
                return;
            }

            self.value()->webview::resolve(id, result);
        };

        auto reject = [shared = m_impl->self, id = message->id](const auto &error)
        {
            auto self = shared->read();

            if (!self.value())
            {
                return;
            }

            self.value()->reject(id, error);
        };

        auto executor = serializer::executor{std::move(resolve), std::move(reject)};
        
        return std::invoke(*exposed, std::move(message), std::move(executor));
    }

    void smartview_core::resolve(std::unique_ptr<result_data> message)
    {
        resolver evaluation;

        if (auto locked = m_impl->evaluations.write(); auto node = locked->extract(message->id))
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

    void smartview_core::add_evaluation(resolver &&resolve, const std::string &code)
    {
        auto id = m_id_counter++;

        {
            auto locked = m_impl->evaluations.write();
            locked->emplace(id, std::move(resolve));
        }

        webview::execute(fmt::format(
            R"(
                (async () =>
                    window.saucer.internal.resolve({}, {})
                )();
            )",
            id, code));
    }

    void smartview_core::clear_exposed()
    {
        auto locked = m_impl->functions.write();
        locked->clear();
    }

    void smartview_core::clear_exposed(const std::string &name)
    {
        auto locked = m_impl->functions.write();
        locked->erase(name);
    }
} // namespace saucer
