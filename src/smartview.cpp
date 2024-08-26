#include "smartview.hpp"

#include "latch.hpp"
#include "scripts.hpp"

#include "serializers/data.hpp"
#include "serializers/serializer.hpp"

#include <rebind/enum.hpp>
#include <lockpp/lock.hpp>

#include <fmt/core.h>
#include <poolparty/pool.hpp>

namespace saucer
{
    using lockpp::lock;

    using resolver = saucer::serializer::resolver;
    using function = saucer::serializer::function;

    struct smartview_core::impl
    {
        using id             = std::uint64_t;
        using pending_future = std::shared_ptr<std::future<void>>;

      public:
        lock<std::unordered_map<id, resolver>> evaluations;
        lock<std::unordered_map<std::string, std::pair<function, launch>>> functions;

      public:
        shared_latch latch;
        std::unique_ptr<saucer::serializer> serializer;

      public:
        static inline std::unique_ptr<poolparty::pool<>> pool;
    };

    smartview_core::smartview_core(std::unique_ptr<serializer> serializer, const options &options)
        : webview(options), m_impl(std::make_unique<impl>())
    {
        using namespace scripts;

        if (!impl::pool)
        {
            impl::pool = std::make_unique<poolparty::pool<>>(options.threads);
        }

        m_impl->serializer = std::move(serializer);
        auto script        = fmt::format(smartview_script, fmt::arg("serializer", m_impl->serializer->js_serializer()));

        inject({.code = script, .time = load_time::creation, .permanent = true});
        inject({.code = m_impl->serializer->script(), .time = load_time::creation, .permanent = true});
    }

    smartview_core::~smartview_core()
    {
        while (!m_impl->latch.empty())
        {
            run<false>();
        }
    }

    bool smartview_core::on_message(const std::string &message)
    {
        if (webview::on_message(message))
        {
            return true;
        }

        auto parsed = m_impl->serializer->parse(message);

        if (!parsed)
        {
            return false;
        }

        if (auto *data = dynamic_cast<function_data *>(parsed.get()); data)
        {
            call(std::move(parsed));
            return true;
        }

        if (auto *data = dynamic_cast<result_data *>(parsed.get()); data)
        {
            resolve(std::move(parsed));
            return true;
        }

        return false;
    }

    void smartview_core::call(std::unique_ptr<message_data> data)
    {
        const auto &message = *static_cast<function_data *>(data.get());
        auto functions      = m_impl->functions.copy();

        if (!functions.contains(message.name))
        {
            return reject(message.id, fmt::format("\"No exposed function '{}'\"", message.name));
        }

        auto executor = serializer::executor{
            [this, id = message.id, latch = m_impl->latch](const auto &result) { resolve(id, result); },
            [this, id = message.id, latch = m_impl->latch](const auto &error) { reject(id, std::move(error)); },
        };

        auto &[func, policy] = functions.at(message.name);

        if (policy == launch::sync)
        {
            return std::invoke(func, std::move(data), executor);
        }

        m_impl->pool->emplace([func, data = std::move(data), executor = std::move(executor)]() mutable
                              { std::invoke(func, std::move(data), executor); });
    }

    void smartview_core::resolve(std::unique_ptr<message_data> data)
    {
        const auto &message = *static_cast<result_data *>(data.get());
        auto evals          = m_impl->evaluations.write();

        if (!evals->contains(message.id))
        {
            return;
        }

        std::invoke(evals->at(message.id), std::move(data));
        evals->erase(message.id);
    }

    void smartview_core::add_function(std::string name, function &&resolve, launch policy)
    {
        auto functions = m_impl->functions.write();
        functions->emplace(std::move(name), std::make_pair(std::move(resolve), policy));
    }

    void smartview_core::add_evaluation(resolver &&resolve, const std::string &code)
    {
        auto id = m_id_counter++;

        {
            auto locked = m_impl->evaluations.write();
            locked->emplace(id, std::move(resolve));
        }

        execute(fmt::format(
            R"(
                (async () =>
                    window.saucer.internal.resolve({}, {})
                )();
            )",
            id, code));
    }

    void smartview_core::reject(std::uint64_t id, const std::string &reason)
    {
        execute(fmt::format(
            R"(
                window.saucer.internal.rpc[{0}].reject({1});
                delete window.saucer.internal.rpc[{0}];
            )",
            id, reason));
    }

    void smartview_core::resolve(std::uint64_t id, const std::string &result)
    {
        execute(fmt::format(
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
