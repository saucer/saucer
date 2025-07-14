#pragma once

#include "smartview.hpp"

namespace saucer
{
    template <Serializer Serializer>
    smartview<Serializer>::smartview(webview &&webview) : smartview_core(std::move(webview), std::make_unique<Serializer>())
    {
    }

    template <Serializer Serializer>
    smartview<Serializer>::smartview(smartview &&other) noexcept = default;

    template <Serializer Serializer>
    std::optional<smartview<Serializer>> smartview<Serializer>::create(const options &opts)
    {
        auto base = webview::create(opts);

        if (!base.has_value())
        {
            return {};
        }

        return smartview{std::move(base.value())};
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
