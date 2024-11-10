#pragma once

#include "handle.hpp"
#include "ref_obj.hpp"

#import <MacTypes.h>

namespace saucer::utils
{
    template <typename T>
    constexpr auto objc_retain(T ptr)
    {
        [ptr retain];
    };

    template <typename T>
    constexpr auto objc_release(T ptr)
    {
        [ptr release];
    };

    template <typename T>
    using objc_obj = ref_obj<T, objc_retain<T>, objc_release<T>>;

    template <typename T>
    using objc_ptr = objc_obj<T *>;

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
