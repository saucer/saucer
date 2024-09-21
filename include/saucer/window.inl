#pragma once

#include "window.hpp"

namespace saucer
{
    template <typename Callback>
    auto window::dispatch(Callback &&callback) const
    {
        return m_parent->dispatch(std::forward<Callback>(callback)).get();
    }
} // namespace saucer
