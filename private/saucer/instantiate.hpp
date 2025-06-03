#pragma once

#include <tuple>
#include <utility>

#define SAUCER_INSTANTIATE_EVENTS(class, type, n)                                                                                     \
    template <typename>                                                                                                               \
    struct instantiate_##class##_##type                                                                                               \
    {                                                                                                                                 \
    };                                                                                                                                \
                                                                                                                                      \
    template <std::size_t... N>                                                                                                       \
    struct instantiate_##class##_##type<std::index_sequence<N...>>                                                                    \
    {                                                                                                                                 \
        static constexpr auto table =                                                                                                 \
            std::make_tuple(std::make_pair(&class ::on<static_cast<type>(N)>, &class ::once<static_cast<type>(N)>)...);               \
    };                                                                                                                                \
                                                                                                                                      \
    template struct instantiate_##class##_##type<std::make_index_sequence<n>>;

#define SAUCER_INSTANTIATE_WINDOW_EVENTS SAUCER_INSTANTIATE_EVENTS(window, window_event, 7)
#define SAUCER_INSTANTIATE_WEBVIEW_EVENTS SAUCER_INSTANTIATE_EVENTS(webview, web_event, 6)
