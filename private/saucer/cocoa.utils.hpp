#pragma once

#include "ref_obj.hpp"

#include <string>

#import <Foundation/Foundation.h>

namespace saucer::utils
{
    template <typename T>
    constexpr void objc_retain(T);

    template <typename T>
    constexpr void objc_release(T);

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

    [[nodiscard]] NSUUID *uuid_from(const std::string &);
} // namespace saucer::utils

#include "cocoa.utils.inl"
