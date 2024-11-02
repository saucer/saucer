#pragma once

#include "smartview.hpp"

namespace saucer
{
    template <Serializer Serializer>
    smartview<Serializer>::smartview(const preferences &prefs) : smartview_core(std::make_unique<Serializer>(), prefs)
    {
    }

    template <Serializer Serializer>
    template <typename... Params>
    void smartview<Serializer>::execute(std::string_view code, Params &&...params)
    {
        auto args = Serializer::serialize_args(std::forward<Params>(params)...);
        webview::execute(fmt::vformat(code, args));
    }

    template <Serializer Serializer>
    template <typename Return, typename... Params>
    std::future<Return> smartview<Serializer>::evaluate(std::string_view code, Params &&...params)
    {
        std::promise<Return> promise;
        auto rtn = promise.get_future();

        auto args    = Serializer::serialize_args(std::forward<Params>(params)...);
        auto resolve = Serializer::resolve(std::move(promise));

        add_evaluation(std::move(resolve), fmt::vformat(code, args));

        return rtn;
    }

    template <Serializer Serializer>
    template <typename Function>
    void smartview<Serializer>::expose(std::string name, Function &&func, launch policy)
    {
        auto resolve = Serializer::serialize(std::forward<Function>(func));
        add_function(std::move(name), std::move(resolve), policy);
    }
} // namespace saucer
