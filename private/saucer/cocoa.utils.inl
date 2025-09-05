#pragma once

#include "cocoa.utils.hpp"

#import <MacTypes.h>

extern "C"
{
    void *objc_autoreleasePoolPush(void);
    void objc_autoreleasePoolPop(void *pool);
}

namespace saucer
{
    template <typename T>
    constexpr void utils::objc_retain(T ptr)
    {
        [ptr retain];
    };

    template <typename T>
    constexpr void utils::objc_release(T ptr)
    {
        [ptr release];
    };

    inline utils::autorelease_guard::autorelease_guard() : m_pool(objc_autoreleasePoolPush()) {}

    inline utils::autorelease_guard::~autorelease_guard()
    {
        objc_autoreleasePoolPop(m_pool);
    }
} // namespace saucer
