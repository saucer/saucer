#include "smartview.hpp"

#include "serializers/data.hpp"
#include "serializers/serializer.hpp"

#include <fmt/core.h>

#include <rebind/enum.hpp>
#include <lockpp/lock.hpp>

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
        lock<std::map<id, resolver>> evaluations;
        lock<std::map<std::string, function>> functions;

      public:
        std::unique_ptr<saucer::serializer> serializer;
    };

    smartview_core::smartview_core(std::unique_ptr<serializer> serializer, const options &options)
        : webview(options), m_impl(std::make_unique<impl>())
    {
        m_impl->serializer = std::move(serializer);

        inject(fmt::format(R"js(
        window.saucer._idc = 0;
        window.saucer._rpc = [];
        
        window.saucer.call = async (name, params) =>
        {{
            if (!Array.isArray(params))
            {{
                throw 'Bad arguments, expected array';
            }}

            if (typeof name !== 'string' && !(name instanceof String))
            {{
                throw 'Bad name, expected string';
            }}

            const id = ++window.saucer._idc;
            
            const rtn = new Promise((resolve, reject) => {{
                window.saucer._rpc[id] = {{
                    reject,
                    resolve,
                }};
            }});

            await window.saucer.on_message({serializer}({{
                    id,
                    name,
                    params,
            }}));

            return rtn;
        }}

        window.saucer.exposed = new Proxy({{}}, {{
            get: (_, prop) => (...args) => window.saucer.call(prop, args),
        }})

        window.saucer._resolve = async (id, value) =>
        {{
            await window.saucer.on_message({serializer}({{
                    id,
                    result: value === undefined ? null : value,
            }}));
        }}
        )js",
                           fmt::arg("serializer", m_impl->serializer->js_serializer())),
               load_time::creation);

        inject(m_impl->serializer->script(), load_time::creation);
    }

    smartview_core::~smartview_core() = default;

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

        if (auto *message = dynamic_cast<function_data *>(parsed.get()); message)
        {
            auto functions = m_impl->functions.copy();

            if (!functions.contains(message->name))
            {
                reject(message->id, error{
                                        error_code::unknown_function,
                                        fmt::format("No exposed function '{}'", message->name),
                                    });

                return false;
            }

            auto executor = serializer::executor{
                [this, id = message->id](const auto &result) { resolve(id, result); },
                [this, id = message->id](const auto &error) { reject(id, std::move(error)); },
            };

            std::invoke(functions.at(message->name), *message, executor);
        }

        if (auto *message = dynamic_cast<result_data *>(parsed.get()); message)
        {
            auto evals = m_impl->evaluations.write();

            if (!evals->contains(message->id))
            {
                return false;
            }

            std::invoke(evals->at(message->id), *message);
            evals->erase(message->id);

            return true;
        }

        return false;
    }

    void smartview_core::add_function(std::string name, function &&resolve)
    {
        auto functions = m_impl->functions.write();
        functions->emplace(std::move(name), std::move(resolve));
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
                    window.saucer._resolve({}, {})
                )();
            )",
            id, code));
    }

    void smartview_core::reject(std::uint64_t id, error error)
    {
        const auto meta = rebind::enum_value(error.ec);
        auto message    = fmt::format("{}", meta ? meta->name : "<Unknown>");

        if (!error.message.empty())
        {
            message = fmt::format("{}: {}", message, error.message);
        }

        std::ranges::replace(message, '"', '\'');

        execute(fmt::format(
            R"(
                window.saucer._rpc[{0}].reject("{1}");
                delete window.saucer._rpc[{0}];
            )",
            id, message));
    }

    void smartview_core::resolve(std::uint64_t id, const std::string &result)
    {
        execute(fmt::format(
            R"(
                window.saucer._rpc[{0}].resolve({1});
                delete window.saucer._rpc[{0}];
            )",
            id, result));
    }
} // namespace saucer
