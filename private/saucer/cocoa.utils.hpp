#pragma once

#include <saucer/utils/ref_ptr.hpp>

namespace saucer::utils
{
    template <typename T>
    constexpr auto objc_retain(T *ptr)
    {
        [ptr retain];
    };

    template <typename T>
    constexpr auto objc_release(T *ptr)
    {
        [ptr release];
    };

    template <typename T>
    using objc_ptr = ref_ptr<T, objc_retain<T>, objc_release<T>>;

    class [[maybe_unused]] autorelease_guard
    {
        void *m_pool;

      public:
        autorelease_guard();

      public:
        ~autorelease_guard();
    };
} // namespace saucer::utils

#include "cocoa.utils.inl"
