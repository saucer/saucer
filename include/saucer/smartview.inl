#pragma once

#include "smartview.hpp"

namespace saucer
{
    template <Serializer Serializer>
    basic_smartview<Serializer>::basic_smartview(webview &&webview) : smartview_base(std::move(webview), std::make_unique<Serializer>())
    {
    }

    template <Serializer Serializer>
    basic_smartview<Serializer>::basic_smartview(basic_smartview &&other) noexcept = default;

    template <Serializer Serializer>
    result<basic_smartview<Serializer>> basic_smartview<Serializer>::create(const options &opts)
    {
        auto base = webview::create(opts);

        if (!base.has_value())
        {
            return err(base);
        }

        return basic_smartview{std::move(base.value())};
    }

    template <Serializer Serializer>
    template <typename... Ts>
    void basic_smartview<Serializer>::execute(format_string<Serializer, Ts...> code, Ts &&...params)
    {
        webview::execute(std::format(code, Serializer::serialize(std::forward<Ts>(params))...));
    }

    template <Serializer Serializer>
    template <typename R, typename... Ts>
    auto basic_smartview<Serializer>::evaluate(format_string<Serializer, Ts...> code, Ts &&...params)
    {
        using result = std::conditional_t<std::is_void_v<R>, void, serializer_core::result<R>>;

        auto promise = coco::promise<result>{};
        auto rtn     = promise.get_future();

        auto format  = std::format(code, Serializer::serialize(std::forward<Ts>(params))...);
        auto resolve = Serializer::resolve(std::move(promise));

        add_evaluation(std::move(resolve), format);

        return rtn;
    }

    template <Serializer Serializer>
    template <typename Function>
    void basic_smartview<Serializer>::expose(std::string name, Function &&func)
    {
        auto resolve = Serializer::convert(std::forward<Function>(func));
        add_function(std::move(name), std::move(resolve));
    }
} // namespace saucer
