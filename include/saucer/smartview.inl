#pragma once
#include "smartview.hpp"

namespace saucer
{
    template <Serializer Serializer>
    smartview<Serializer>::smartview(const options &options) : smartview_core(std::make_unique<Serializer>(), options)
    {
    }

    template <Serializer Serializer>
    template <typename Return, typename... Params>
    std::future<Return> smartview<Serializer>::evaluate(const std::string &code, Params &&...params)
    {
        auto promise = std::make_shared<std::promise<Return>>();
        auto rtn = promise->get_future();

        auto args = Serializer::serialize_args(std::forward<Params>(params)...);
        auto resolve = Serializer::resolve(promise);

        add_evaluation(std::move(resolve), fmt::vformat(code, args));

        return rtn;
    }

    template <Serializer Serializer>
    template <typename Function>
    void smartview<Serializer>::expose(std::string name, const Function &func, bool async)
    {
        auto resolve = Serializer::serialize(func);
        add_function(std::move(name), std::move(resolve), async);
    }
} // namespace saucer