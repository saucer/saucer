#pragma once

#include <memory>

namespace saucer::utils
{
    namespace detail
    {
        template <typename T>
        struct deferred_delete;
    }

    template <typename T>
    using deferred_ptr = std::unique_ptr<T, detail::deferred_delete<T>>;

    template <typename T, typename... Ts>
    auto make_deferred(Ts &&...);
} // namespace saucer::utils

#include "qt.utils.inl"
