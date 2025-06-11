#pragma once

#include "../module.hpp"

namespace saucer::modules
{
    template <typename>
    class traits;

    template <typename T>
    using extend = extensible<T, typename traits<T>::interface>;
} // namespace saucer::modules
