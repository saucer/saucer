#include "webview.hpp"
#include <smartview.hpp>

namespace saucer
{
    smartview::~smartview() = default;
    smartview::smartview()
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
            if (parsed_message)
            {
                if (!m_callbacks.count(parsed_message->function))
                {
                    reject(parsed_message->id, "\"Invalid function\"");
                    return;
                }

                auto result = m_callbacks.at(parsed_message->function)(parsed_message);
                if (result.has_value())
                {
                    resolve(parsed_message->id, *result);
                    return;
                }

                switch (result.error())
                {
                case serializer::error::argument_mismatch:
                    reject(parsed_message->id, "\"Invalid arguments\"");
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