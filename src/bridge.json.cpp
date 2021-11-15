#include <bridge/json/bridge.hpp>

namespace saucer
{
    json_bridge::json_bridge()
    {
        inject(R"js(
        window.saucer._idc = 0;
        window.saucer._rpc = [];
        
        window.saucer.call = async (name, params) =>
        {
            if (Array.isArray(params) && (typeof name === 'string' || name instanceof String))
            {
                const id = ++window.saucer._idc;
                const rtn = new Promise((resolve, reject) => 
                {
                    window.saucer._rpc[id] = {
                        reject,
                        resolve,
                    };
                });

                await window.saucer.on_message(JSON.stringify({
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

        window.saucer._resolve = async (id, result) =>
        {
            await window.saucer.on_message(JSON.stringify({
                id,
                result,
            }));
        }
        )js",
               load_time_t::creation);
    }

    void json_bridge::on_message(const std::string &message)
    {
        auto data = nlohmann::json::parse(message, nullptr, false);
        if (!data.is_discarded())
        {
            if (data["id"].is_number_integer() && data["name"].is_string() && data["params"].is_array())
            {
                const int id = data["id"];
                const std::string name = data["name"];
                const auto params = data["params"];

                if (m_callbacks.count(name))
                {
                    m_callbacks[name](id, params);
                }
            }
            else if (data["id"].is_number() && data.contains("result"))
            {
                const int id = data["id"];
                const auto result = data["result"];

                if (m_promises.count(id))
                {
                    m_promises[id]->resolve(result);
                    m_promises.erase(id);
                }
            }
        }
    }

    void json_bridge::reject(int id, const nlohmann::json &data)
    {
        //? We dump twice to properly escape everything.

        // clang-format off
        run_java_script(
            "window.saucer._rpc[" + std::to_string(id) + "].reject(JSON.parse(" + nlohmann::json(data.dump()).dump() + "));\n" 
                        "delete window.saucer._rpc["+ std::to_string(id) + "]"
        );
        // clang-format on
    }

    void json_bridge::resolve(int id, const nlohmann::json &data)
    {
        // clang-format off
        run_java_script(
            "window.saucer._rpc[" + std::to_string(id) + "].resolve(JSON.parse(" + nlohmann::json(data.dump()).dump()  + "));\n"
                        "delete window.saucer._rpc["+ std::to_string(id) + "]"
        );
        // clang-format on
    }
} // namespace saucer