#include "smartview.hpp"
#include "serializers/serializer.hpp"

#include <cassert>

namespace saucer
{
    smartview::~smartview() = default;
    smartview::smartview() : m_creation_thread(std::this_thread::get_id())
    {
        inject(R"js(
        window.saucer._idc = 0;
        window.saucer._rpc = [];
        window.saucer._known_functions = new Map();
        
        window.saucer.call = async (name, params, serializer = null) =>
        {
            if (Array.isArray(params) && (typeof name === 'string' || name instanceof String))
            {
                if (serializer == null)
                {
                    if (window.saucer._known_functions.has(name))
                    {
                        serializer = window.saucer._known_functions.get(name);
                    }
                    else
                    {
                        throw "Unable to automatically determine the serializer for this function. Please provide it manually."; 
                    }
                }

                const id = ++window.saucer._idc;
                const rtn = new Promise((resolve, reject) => 
                {
                    window.saucer._rpc[id] = {
                        reject,
                        resolve,
                    };
                });

                await window.saucer.on_message(serializer({
                    id,
                    name,
                    params,
                }));

                return rtn;
            }
            else
            {
                throw "Invalid arguments";
            }
        }

        window.saucer._resolve = async (id, value, serializer = null) =>
        {
            await window.saucer.on_message(serializer({
                id,
                result: value === undefined ? null : value,
            }));
        }
        )js",
               load_time::creation);
    }

    void smartview::resolve(const std::shared_ptr<function_data> &data, const callback_resolver &callback)
    {
        auto result = callback(data);
        if (result.has_value())
        {
            resolve(data->id, *result);
        }
        else
        {
            switch (result.error())
            {
            case serializer::error::argument_count_mismatch:
                reject(data->id, "\"Argument Count Mismatch\"");
                break;
            case serializer::error::type_mismatch:
                reject(data->id, "\"Type Mismatch\"");
                break;
            }
        }
    }

    void smartview::on_url_changed(const std::string &url)
    {
        for (const auto &module : m_modules)
        {
            module->on_url_changed(url);
        }

        webview::on_url_changed(url);
    }

    void smartview::on_message(const std::string &message)
    {
        webview::on_message(message);

        for (const auto &module : m_modules)
        {
            module->on_message(message);
        }

        for (const auto &[serializer_type, serializer] : m_serializers)
        {
            auto parsed_message = serializer->parse(message);

            if (!parsed_message)
                continue;

            if (auto function_message = std::dynamic_pointer_cast<function_data>(parsed_message); function_message)
            {
                if (!m_callbacks.count(function_message->function))
                {
                    reject(function_message->id, "\"Invalid function\"");
                    return;
                }

                const auto &[async, callback, function_serializer_type] = m_callbacks.at(function_message->function);

                if (serializer_type != function_serializer_type)
                {
                    continue;
                }

                if (async)
                {
                    auto fut_ptr = std::make_shared<std::future<void>>();
                    *fut_ptr = std::async(std::launch::async, [fut_ptr, callback = callback, function_message, this] { resolve(function_message, callback); });
                    return;
                }

                resolve(function_message, callback);
                return;
            }
            if (auto result_message = std::dynamic_pointer_cast<result_data>(parsed_message); result_message)
            {
                auto locked_evals = m_evals.write();

                if (!locked_evals->count(result_message->id))
                {
                    return;
                }

                const auto &[resolver, eval_serialier_type, promise] = locked_evals->at(result_message->id);
                auto result = resolver(result_message);

                if (serializer_type != eval_serialier_type)
                {
                    continue;
                }

                if (!result.has_value())
                {
                    promise->reject(result.error());
                }

                locked_evals->erase(result_message->id);
                return;
            }
        }
    }

    void smartview::add_callback(const std::type_index &serializer, const std::string &name, const callback_resolver &resolver, bool async)
    {
        m_callbacks.emplace(name, callback_t{async, resolver, serializer});
        inject("window.saucer._known_functions.set(\"" + name + "\", " + m_serializers.at(serializer)->java_script_serializer() + ");", load_time::creation);
    }

    void smartview::add_eval(const std::type_index &serializer, const std::shared_ptr<promise_base> &promise, const eval_resolver &resolver, const std::string &code)
    {
        auto id = m_id_counter++;
        m_evals.write()->emplace(id, eval_t{resolver, serializer, promise});

        auto resolve_code = "(async () => window.saucer._resolve(" + std::to_string(id) + "," + code + ", " + m_serializers.at(serializer)->java_script_serializer() + "))();";
        run_java_script(resolve_code);
    }

    void smartview::resolve(const std::size_t &id, const std::string &result)
    {
        // clang-format off
        run_java_script("window.saucer._rpc[" + std::to_string(id) + "].resolve(" + result + ");\n"
                        "delete window.saucer._rpc[" + std::to_string(id) + "];");
        // clang-format on
    }

    void smartview::reject(const std::size_t &id, const std::string &result)
    {
        // clang-format off
        run_java_script("window.saucer._rpc[" + std::to_string(id) + "].reject(" + result + ");\n"
                        "delete window.saucer._rpc[" + std::to_string(id) + "];");
        // clang-format on
    }

    std::vector<module *> smartview::get_modules()
    {
        std::vector<module *> rtn;
        std::transform(m_modules.begin(), m_modules.end(), std::back_inserter(rtn), [](const auto &item) { return item.get(); });

        return rtn;
    }

    module *smartview::get_module(const std::string &name)
    {
        for (const auto &module : m_modules)
        {
            if (module->get_name() == name)
            {
                return module.get();
            }
        }
        return nullptr;
    }
} // namespace saucer