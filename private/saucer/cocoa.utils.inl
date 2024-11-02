#pragma once

#include "cocoa.utils.hpp"

extern "C"
{
    void *objc_autoreleasePoolPush(void);
    void objc_autoreleasePoolPop(void *pool);
}

namespace saucer::utils
{
    inline autorelease_guard::autorelease_guard() : m_pool(objc_autoreleasePoolPush()) {}

    inline autorelease_guard::~autorelease_guard()
    {
        objc_autoreleasePoolPop(m_pool);
    }
} // namespace saucer::utils
