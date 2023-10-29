#pragma once

#include <concepts>

namespace saucer
{
    class smartview_core;

    namespace native
    {
        struct window;
        struct webview;
    } // namespace native

    class module
    {
      protected:
        smartview_core *core;

      public:
        virtual ~module();

      public:
        module(smartview_core *);

      protected:
        virtual void init(native::window *, native::webview *) = 0;
    };

    template <typename T>
    concept Module = requires() {
        requires std::derived_from<T, module>;
        requires std::constructible_from<T, smartview_core *>;
    };
} // namespace saucer
