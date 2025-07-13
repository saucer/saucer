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

#define SAUCER_INSTANTIATE_WINDOW_EVENT(EVENT)      SAUCER_INSTANTIATE_EVENT(window, EVENT)
#define SAUCER_INSTANTIATE_WINDOW_IMPL_EVENT(EVENT) SAUCER_INSTANTIATE_EVENT(window::impl, EVENT)

#define SAUCER_INSTANTIATE_WINDOW_EVENTS(MACRO)                                                                                       \
    SAUCER_RECURSE(MACRO, window::event::decorated, window::event::maximize, window::event::minimize, window::event::closed,          \
                   window::event::resize, window::event::focus, window::event::close)

#define SAUCER_INSTANTIATE_WEBVIEW_EVENT(EVENT)      SAUCER_INSTANTIATE_EVENT(webview, EVENT)
#define SAUCER_INSTANTIATE_WEBVIEW_IMPL_EVENT(EVENT) SAUCER_INSTANTIATE_EVENT(webview::impl, EVENT)

#define SAUCER_INSTANTIATE_WEBVIEW_EVENTS(MACRO)                                                                                      \
    SAUCER_RECURSE(MACRO, webview::event::permission, webview::event::dom_ready, webview::event::navigated, webview::event::navigate, \
                   webview::event::request, webview::event::favicon, webview::event::title, webview::event::load)
