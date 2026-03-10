#pragma once

#include "stash.impl.hpp"

namespace saucer
{
    template <typename T>
    std::size_t stash::impl::id_of()
    {
        static const auto rtn = count();
        return rtn;
    }
} // namespace saucer
