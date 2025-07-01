#pragma once

#define SAUCER_RESCAN_(...) SAUCER_RESCAN2(SAUCER_RESCAN2(SAUCER_RESCAN2(SAUCER_RESCAN2(__VA_ARGS__))))
#define SAUCER_RESCAN2(...) SAUCER_RESCAN1(SAUCER_RESCAN1(SAUCER_RESCAN1(SAUCER_RESCAN1(__VA_ARGS__))))
#define SAUCER_RESCAN1(...) __VA_ARGS__

#define SAUCER_PARENS              ()
#define SAUCER_RECURSE(MACRO, ...) __VA_OPT__(SAUCER_RESCAN_(SAUCER_RECURSE_IMPL(MACRO, __VA_ARGS__)))

#define SAUCER_RECURSE_IMPL(MACRO, ARG, ...)                                                                                          \
    MACRO(ARG)                                                                                                                        \
    __VA_OPT__(SAUCER_RECURSIVE_IMPL2 SAUCER_PARENS(MACRO, __VA_ARGS__))
#define SAUCER_RECURSIVE_IMPL2() SAUCER_RECURSE_IMPL

#define SAUCER_INSTANTIATE_EVENT(CLASS, EVENT) template void CLASS::setup<EVENT>();

#define SAUCER_INSTANTIATE_WINDOW_EVENT(EVENT) SAUCER_INSTANTIATE_EVENT(window, EVENT)
#define SAUCER_INSTANTIATE_WINDOW_EVENTS                                                                                              \
    SAUCER_RECURSE(SAUCER_INSTANTIATE_WINDOW_EVENT, window_event::decorated, window_event::maximize, window_event::minimize,          \
                   window_event::closed, window_event::resize, window_event::focus, window_event::close)

#define SAUCER_INSTANTIATE_WEBVIEW_EVENT(EVENT) SAUCER_INSTANTIATE_EVENT(webview, EVENT)
#define SAUCER_INSTANTIATE_WEBVIEW_EVENTS                                                                                             \
    SAUCER_RECURSE(SAUCER_INSTANTIATE_WEBVIEW_EVENT, web_event::permission, web_event::dom_ready, web_event::navigated,               \
                   web_event::navigate, web_event::request, web_event::favicon, web_event::title, web_event::load)
