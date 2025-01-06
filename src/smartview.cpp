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
        using exposed = std::shared_ptr<std::pair<function, launch>>;

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

        if (std::holds_alternative<std::monostate>(parsed))
        {
            return false;
        }

        if (std::holds_alternative<std::unique_ptr<function_data>>(parsed))
        {
            call(std::move(std::get<0>(parsed)));
            return true;
        }

        if (std::holds_alternative<std::unique_ptr<result_data>>(parsed))
        {
            resolve(std::move(std::get<1>(parsed)));
            return true;
        }

        return false;
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

            self.value()->resolve(id, result);
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

        auto executor     = serializer::executor{std::move(resolve), std::move(reject)};
        const auto policy = exposed->second;

        if (policy == launch::sync)
        {
            return std::invoke(exposed->first, std::move(message), executor);
        }

        m_parent->pool().emplace(
            [exposed = std::move(exposed), message = std::move(message), executor = std::move(executor)]() mutable
            { std::invoke(exposed->first, std::move(message), executor); });
    }

    void smartview_core::resolve(std::unique_ptr<result_data> message)
    {
        const auto id = message->id;
        auto evals    = m_impl->evaluations.write();

        if (!evals->contains(id))
        {
            return;
        }

        std::invoke(evals->at(id), std::move(message));
        evals->erase(id);
    }

    void smartview_core::add_function(std::string name, function &&resolve, launch policy)
    {
        auto functions = m_impl->functions.write();
        functions->emplace(std::move(name), std::make_shared<impl::exposed::element_type>(std::move(resolve), policy));
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

    void smartview_core::reject(std::uint64_t id, const std::string &reason)
    {
        webview::execute(fmt::format(
            R"(
                window.saucer.internal.rpc[{0}].reject({1});
                delete window.saucer.internal.rpc[{0}];
            )",
            id, reason));
    }

    void smartview_core::resolve(std::uint64_t id, const std::string &result)
    {
        webview::execute(fmt::format(
            R"(
                window.saucer.internal.rpc[{0}].resolve({1});
                delete window.saucer.internal.rpc[{0}];
            )",
            id, result));
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
