#include "webview.hpp"
#include "smartview.hpp"
#include "serializers/serializer.hpp"

#include <fmt/format.h>

namespace saucer
{
    smartview::smartview(const webview_options &options) : webview(options)
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

    smartview::~smartview()
    {
        auto futures = m_futures.read()->size();

        while (futures)
        {
            run<false>();
            futures = m_futures.read()->size();
        }
    }

    void smartview::resolve(function_data &data, const resolve_callback &callback)
    {
        auto result = callback(data);

        if (result.has_value())
        {
            resolve(data.id, *result);
            return;
        }

        switch (result.error())
        {
        case serializer::error::argument_count_mismatch:
            reject(data.id, R"("Argument Count Mismatch")");
            break;

        case serializer::error::type_mismatch:
            reject(data.id, R"("Type Mismatch")");
            break;
        }
    }

    void smartview::on_message(const std::string &message)
    {
        auto callbacks = m_callbacks.read();
        auto serializers = m_serializers.read();

        for (const auto &[serializer_type, serializer] : *serializers)
        {
            auto parsed = serializer->parse(message);

            if (!parsed)
                continue;

            if (dynamic_cast<function_data *>(parsed.get()))
            {
                auto &function_message = dynamic_cast<function_data &>(*parsed);

                if (!callbacks->count(function_message.function))
                {
                    reject(function_message.id, R"("Invalid function")");
                    return;
                }

                const auto &[async, callback, function_serializer_type] = callbacks->at(function_message.function);

                if (serializer_type != function_serializer_type)
                {
                    continue;
                }

                if (async)
                {
                    /*
                    ? As it is completely "legal" to call "smartview.close()" inside of an async callback we have to
                    ? ensure that all futures are properly processed before we close the smartview, as not doing so
                    ? would cause a segmentation fault (more precisely: when a callback calls close the std::async which
                    ? executes the callback will then call "resolve" with the captured result, however, due to the
                    ? smartview potentially being destructed already this causes a segmentation fault).

                    ? To circumvent this we save all futures for async callbacks in our smartview, however we'd like to
                    ? avoid clobbering the memory with old futures.

                    ? The solution to this is the following code, which just creates a shared_ptr to a future,
                    ? then it saves the future, and assigns the async to the future, however the async callback has the
                    ? shared_ptr to the future in it's capture list as a copy, which means that the shared_ptr will have
                    ? a ref_count of 2, once in our smartview future list and once in the lambda capture, the lambda
                    ? capture will destruct itself after execution, so it is completely safe for us to erase the future
                    ? stored in the smartview from within the async's lambda (without causing an exception due to
                    ? destructing a non ready future).

                    ? On destruction of the smartview we then check for any remaining futures, and call the non blocking
                    ? "run" method of the window to ensure that all callbacks that were safely queued (e.g. by
                    ? "postSafe") are properly executed, once they're executed they will erase themselves from the
                    ? futures list.
                    */

                    auto fut = std::make_shared<std::future<void>>();
                    auto fut_id = m_id_counter++;

                    m_futures.write()->emplace(fut_id, fut);

                    *fut = std::async(std::launch::async,
                                      [this, fut, fut_id, callback = callback, parsed = std::move(parsed)]() mutable {
                                          auto &message = dynamic_cast<function_data &>(*parsed);

                                          resolve(message, callback);
                                          m_futures.write()->erase(fut_id);
                                      });

                    return;
                }

                resolve(function_message, callback);
                return;
            }

            if (dynamic_cast<result_data *>(parsed.get()))
            {
                auto &result_message = dynamic_cast<result_data &>(*parsed);
                auto evals = m_evals.write();

                if (!evals->count(result_message.id))
                {
                    return;
                }

                const auto &[resolve, eval_serializer_type] = evals->at(result_message.id);

                if (serializer_type != eval_serializer_type)
                {
                    continue;
                }

                resolve(result_message);

                evals->erase(result_message.id);
                return;
            }
        }

        webview::on_message(message);
    }

    void smartview::add_eval(std::type_index type, eval_callback &&resolve, const std::string &code)
    {
        auto evals = m_evals.write();
        auto serializers = m_serializers.read();

        auto id = m_id_counter++;
        evals->emplace(id, eval_t{std::move(resolve), type});
        const auto &serializer = serializers->at(type);

        run_java_script(fmt::format(
            R"(
                (async () =>
                    window.saucer._resolve({}, {}, {})
                )();
            )",
            id, code, serializer->js_serializer()));
    }

    void smartview::add_callback(std::type_index type, const std::string &name, resolve_callback &&resolve, bool async)
    {
        auto callbacks = m_callbacks.write();
        auto serializers = m_serializers.read();

        callbacks->emplace(name, callback_t{async, std::move(resolve), type});
        const auto &serializer = serializers->at(type);

        inject(fmt::format(
                   R"(
                        window.saucer._known_functions.set("{}", {});
                    )",
                   name, serializer->js_serializer()),
               load_time::creation);
    }

    void smartview::resolve(std::size_t id, const std::string &result)
    {
        run_java_script(fmt::format(
            R"(
                window.saucer._rpc[{0}].resolve({1});
                delete window.saucer._rpc[{0}];
            )",
            id, result));
    }

    void smartview::reject(std::size_t id, const std::string &result)
    {
        run_java_script(fmt::format(
            R"(
                window.saucer._rpc[{0}].reject({1});
                delete window.saucer._rpc[{0}];
            )",
            id, result));
    }
} // namespace saucer