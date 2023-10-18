#include "smartview.hpp"
#include "serializers/data.hpp"
#include "serializers/serializer.hpp"

#include <regex>
#include <fmt/core.h>

namespace saucer
{
    using lockpp::lock;

    struct exposed_function
    {
        bool async;
        serializer::function function;
    };

    struct smartview_core::impl
    {
        using id             = std::uint64_t;
        using pending_future = std::shared_ptr<std::future<void>>;

      public:
        lock<std::map<id, pending_future>> pending;

      public:
        lock<std::map<std::string, exposed_function>> functions;
        lock<std::map<id, saucer::serializer::resolver>> evaluations;

      public:
        std::unique_ptr<saucer::serializer> serializer;
    };

    smartview_core::smartview_core(std::unique_ptr<serializer> serializer, const options &options)
        : webview(options), m_impl(std::make_unique<impl>())
    {
        m_impl->serializer = std::move(serializer);

        inject(std::regex_replace(R"js(
        window.saucer._idc = 0;
        window.saucer._rpc = [];
        
        window.saucer.call = async (name, params) =>
        {
            if (!Array.isArray(params))
            {
                throw "Bad Arguments, expected array";
            }

            if (typeof name !== 'string' && !(name instanceof String))
            {
                throw "Bad Name, expected string";
            }

            const id = ++window.saucer._idc;
            
            const rtn = new Promise((resolve, reject) => {
                window.saucer._rpc[id] = {
                    reject,
                    resolve,
                };
            });

            await window.saucer.on_message(<serializer>({
                    id,
                    name,
                    params,
            }));

            return rtn;
        }

        window.saucer._resolve = async (id, value) =>
        {
            await window.saucer.on_message(<serializer>({
                    id,
                    result: value === undefined ? null : value,
            }));
        }
        )js",
                                  std::regex{"<serializer>"}, m_impl->serializer->js_serializer()),
               load_time::creation);

        inject(m_impl->serializer->script(), load_time::creation);
    }

    smartview_core::~smartview_core()
    {
        auto finished = m_impl->pending.read()->empty();

        while (!finished)
        {
            run<false>();
            finished = m_impl->pending.read()->empty();
        }
    }

    void smartview_core::call(function_data &data, const serializer::function &callback)
    {
        auto result = callback(data);

        if (result.has_value())
        {
            resolve(data.id, *result);
            return;
        }

        reject(data.id, result.error());
    }

    void smartview_core::on_message(const std::string &message)
    {
        auto parsed = m_impl->serializer->parse(message);

        if (!parsed)
        {
            webview::on_message(message);
            return;
        }

        if (auto *message = dynamic_cast<function_data *>(parsed.get()); message)
        {
            auto functions = m_impl->functions.copy();

            if (!functions.contains(message->name))
            {
                reject(message->id, serializer_error::invalid_function);
                return;
            }

            const auto &[async, callback] = functions.at(message->name);

            if (!async)
            {
                call(*message, callback);
                return;
            }

            auto id     = m_id_counter++;
            auto future = std::make_shared<impl::pending_future::element_type>();

            m_impl->pending.write()->emplace(id, future);

            auto fn = [this, future, id, callback, parsed = std::move(parsed)]()
            {
                auto *message = static_cast<function_data *>(parsed.get());

                call(*message, callback);
                m_impl->pending.write()->erase(id);
            };

            *future = std::async(std::launch::async, std::move(fn));
            return;
        }

        if (auto *message = dynamic_cast<result_data *>(parsed.get()); message)
        {
            auto evals = m_impl->evaluations.write();

            if (!evals->contains(message->id))
            {
                return;
            }

            const auto &resolve = evals->at(message->id);
            resolve(*message);

            evals->erase(message->id);
            return;
        }
    }

    void smartview_core::add_function(std::string name, serializer::function &&resolve, bool async)
    {
        auto functions = m_impl->functions.write();
        functions->emplace(std::move(name), exposed_function{async, std::move(resolve)});
    }

    void smartview_core::add_evaluation(serializer::resolver &&resolve, const std::string &code)
    {
        auto id = m_id_counter++;
        m_impl->evaluations.write()->emplace(id, std::move(resolve));

        run_java_script(fmt::format(
            R"(
                (async () =>
                    window.saucer._resolve({}, {})
                )();
            )",
            id, code));
    }

    void smartview_core::reject(std::uint64_t id, serializer_error error)
    {
        using underlying = std::underlying_type_t<serializer_error>;

        run_java_script(fmt::format(
            R"(
                window.saucer._rpc[{0}].reject("{1}");
                delete window.saucer._rpc[{0}];
            )",
            id, static_cast<underlying>(error)));
    }

    void smartview_core::resolve(std::uint64_t id, const std::string &result)
    {
        run_java_script(fmt::format(
            R"(
                window.saucer._rpc[{0}].resolve({1});
                delete window.saucer._rpc[{0}];
            )",
            id, result));
    }
} // namespace saucer
