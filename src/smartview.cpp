#include <smartview.hpp>

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
                        serializer =  window.saucer._known_functions.get(name);
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
               load_time_t::creation);
    }

    void smartview::on_message(const std::string &message)
    {
        webview::on_message(message);

        std::function<void()> on_exit;
        for (const auto &[serializer_type, serializer] : m_serializers)
        {
            auto parsed_message = serializer->parse(message);
            if (auto function_message = std::dynamic_pointer_cast<function_data>(parsed_message); function_message)
            {
                if (!m_callbacks.count(function_message->function))
                {
                    reject(function_message->id, "\"Invalid function\"");
                    return;
                }

                auto result = m_callbacks.at(function_message->function)(function_message);
                if (result.has_value())
                {
                    resolve(function_message->id, *result);
                    return;
                }

                switch (result.error())
                {
                case serializer::error::argument_mismatch:
                    reject(function_message->id, "\"Invalid arguments\"");
                    return;
                case serializer::error::parser_mismatch:
                    continue;
                }
            }
            else if (auto result_message = std::dynamic_pointer_cast<result_data>(parsed_message); result_message)
            {
                if (!m_evals.count(result_message->id))
                {
                    reject(result_message->id, "\"Invalid ID\"");
                    return;
                }

                auto result = m_evals.at(result_message->id).first(result_message);
                if (result.has_value())
                {
                    m_evals.erase(result_message->id);
                    return;
                }

                switch (result.error())
                {
                case serializer::error::argument_mismatch:
                    m_evals.at(result_message->id).second->reject();
                    return;
                case serializer::error::parser_mismatch:
                    continue;
                }
            }
        }
    }

    void smartview::add_callback(const std::shared_ptr<serializer> &serializer, const std::string &name, const callback_t &callback)
    {
        m_callbacks.emplace(name, callback);
        inject("window.saucer._known_functions.set(\"" + name + "\", " + serializer->java_script_serializer() + ");", load_time_t::creation);
    }

    void smartview::add_eval(const std::shared_ptr<serializer> &serializer, const std::shared_ptr<base_promise> &promise, const resolve_callback_t &resolve_callback,
                             const std::string &code, const arg_store_t &params)
    {
        auto id = m_id_counter++;
        m_evals.emplace(id, std::make_pair(resolve_callback, promise));

        auto resolve_code = "(async () => window.saucer._resolve(" + std::to_string(id) + "," + fmt::vformat(code, params) + ", " + serializer->java_script_serializer() + "))();";
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
} // namespace saucer