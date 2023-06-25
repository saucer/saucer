#include "smartview.hpp"
#include "serializers/data.hpp"
#include "serializers/serializer.hpp"

#include <regex>
#include <fmt/core.h>

namespace saucer
{
    using lockpp::lock;

    struct function
    {
        bool async;
        serializer::function function;
    };

    struct smartview_core::impl
    {
      public:
        lock<std::map<std::uint64_t, serializer::promise>> evaluations;

      public:
        lock<std::map<std::uint64_t, std::shared_ptr<std::future<void>>>> pending;
        lock<std::map<std::string, function>> functions;

      public:
        std::unique_ptr<serializer> serializer;
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

        switch (result.error())
        {
        case serializer_error::argument_count_mismatch:
            reject(data.id, "Argument count mismatch");
            break;

        case serializer_error::type_mismatch:
            reject(data.id, "Type mismatch");
            break;
        }
    }

    void smartview_core::on_message(const std::string &message)
    {
        auto parsed = m_impl->serializer->parse(message);

        if (!parsed)
        {
            webview::on_message(message);
            return;
        }

        if (auto *function_message = dynamic_cast<function_data *>(parsed.get()); function_message)
        {
            auto functions = m_impl->functions.copy();

            if (!functions.contains(function_message->name))
            {
                reject(function_message->id, "Invalid function, did you forget to call smartview::expose(...)?");
                return;
            }

            const auto &[async, callback] = functions.at(function_message->name);

            if (!async)
            {
                call(*function_message, callback);
                return;
            }

            auto id = m_id_counter++;
            auto future = std::make_shared<std::future<void>>();

            m_impl->pending.write()->emplace(id, future);

            *future = std::async(std::launch::async, [this, future, id, callback, parsed = std::move(parsed)]() {
                auto &message = dynamic_cast<function_data &>(*parsed);
                call(message, callback);

                m_impl->pending.write()->erase(id);
            });

            return;
        }

        if (auto *result_message = dynamic_cast<result_data *>(parsed.get()); result_message)
        {
            auto evals = m_impl->evaluations.write();

            if (!evals->contains(result_message->id))
            {
                return;
            }

            const auto &resolve = evals->at(result_message->id);
            resolve(*result_message);

            evals->erase(result_message->id);
            return;
        }
    }

    void smartview_core::add_evaluation(serializer::promise &&resolve, const std::string &code)
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

    void smartview_core::add_function(std::string name, serializer::function &&resolve, bool async)
    {
        auto functions = m_impl->functions.write();
        functions->emplace(std::move(name), function{async, std::move(resolve)});
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

    void smartview_core::reject(std::uint64_t id, const std::string &result)
    {
        run_java_script(fmt::format(
            R"(
                window.saucer._rpc[{0}].reject("{1}");
                delete window.saucer._rpc[{0}];
            )",
            id, result));
    }
} // namespace saucer
