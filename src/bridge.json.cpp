#include <bridge/json/json_bridge.hpp>
#include <future>

namespace saucer
{
    namespace bridges
    {
        json::json(webview &webview) : bridge(webview)
        {
            m_webview.inject(R"js(
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
            if (result === undefined)
            {
                await window.saucer.on_message(JSON.stringify({
                    id,
                    result: null,
                }));
            }
            else
            {
                await window.saucer.on_message(JSON.stringify({
                    id,
                    result,
                }));
            }
        }
        )js",
                             load_time_t::creation);
        }

        void json::on_message(const std::string &message)
        {
            auto data = nlohmann::json::parse(message, nullptr, false);
            if (!data.is_discarded())
            {
                if (data["id"].is_number_integer() && data["name"].is_string() && data["params"].is_array())
                {
                    const int id = data["id"];
                    const std::string name = data["name"];
                    const auto params = data["params"];

                    auto locked = m_callbacks.read();
                    if (locked->count(name))
                    {
                        const auto &callback = locked->at(name);
                        if (callback.second)
                        {
                            auto fut = std::make_shared<std::future<void>>();
                            *fut = std::async(std::launch::async, callback.first, id, params);
                        }
                        else
                        {
                            callback.first(id, params);
                        }
                    }
                }
                else if (data["id"].is_number() && data.contains("result"))
                {
                    const int id = data["id"];
                    const auto result = data["result"];

                    auto locked = m_promises.write();
                    if (locked->count(id))
                    {
                        locked->at(id)->resolve(result);
                        locked->erase(id);
                    }
                }
            }
        }

        void json::reject(int id, const nlohmann::json &data)
        {
            //? We dump twice to properly escape everything.

            // clang-format off
            m_webview.run_java_script(
                "window.saucer._rpc[" + std::to_string(id) + "].reject(JSON.parse(" + nlohmann::json(data.dump()).dump() + "));\n" 
                            "delete window.saucer._rpc["+ std::to_string(id) + "]"
            );
            // clang-format on
        }

        void json::resolve(int id, const nlohmann::json &data)
        {
            // clang-format off
            m_webview.run_java_script(
                "window.saucer._rpc[" + std::to_string(id) + "].resolve(JSON.parse(" + nlohmann::json(data.dump()).dump()  + "));\n"
                            "delete window.saucer._rpc["+ std::to_string(id) + "]"
            );
            // clang-format on
        }
    } // namespace bridges
} // namespace saucer