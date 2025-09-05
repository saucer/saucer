#pragma once

#include "qt.utils.hpp"

#include <utility>

namespace saucer::utils
{
    template <typename T>
    struct detail::deferred_delete
    {
        void operator()(T *) const;
    };

    template <typename T>
    void detail::deferred_delete<T>::operator()(T *ptr) const
    {
        ptr->deleteLater();
    }

    template <typename T, typename... Ts>
    auto make_deferred(Ts &&...args)
    {
        return deferred_ptr<T>{new T{std::forward<Ts>(args)...}};
    }
} // namespace saucer::utils
