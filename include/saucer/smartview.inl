#pragma once

#include "smartview.hpp"

namespace saucer
{
    template <Serializer Serializer>
    smartview<Serializer>::smartview(const preferences &prefs) : smartview_core(std::make_unique<Serializer>(), prefs)
    {
    }

    template <Serializer Serializer>
    template <typename... Ts>
    void smartview<Serializer>::execute(format_string<Serializer, Ts...> code, Ts &&...params)
    {
        webview::execute(std::format(code, Serializer::serialize(std::forward<Ts>(params))...));
    }

    template <Serializer Serializer>
    template <typename R, typename... Ts>
    coco::future<R> smartview<Serializer>::evaluate(format_string<Serializer, Ts...> code, Ts &&...params)
    {
        auto promise = coco::promise<R>{};
        auto rtn     = promise.get_future();

        auto format  = std::format(code, Serializer::serialize(std::forward<Ts>(params))...);
        auto resolve = Serializer::resolve(std::move(promise));

        add_evaluation(std::move(resolve), format);

        return rtn;
    }

    template <Serializer Serializer>
    template <typename Function>
    void smartview<Serializer>::expose(std::string name, Function &&func)
    {
        auto resolve = Serializer::convert(std::forward<Function>(func));
        add_function(std::move(name), std::move(resolve));
    }
} // namespace saucer
