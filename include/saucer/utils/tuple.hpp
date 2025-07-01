#pragma once

namespace saucer::tuple
{
    template <typename T>
    struct is_tuple;

    template <typename T>
    concept Tuple = is_tuple<T>::value;

    template <typename T, template <typename...> typename Transform>
    struct transform;

    template <typename T, template <typename...> typename Transform>
    using transform_t = transform<T, Transform>::type;

    template <typename T>
    struct drop_last;

    template <typename T>
    using drop_last_t = drop_last<T>::type;

    template <typename T>
    struct last;

    template <typename T>
    using last_t = last<T>::type;
} // namespace saucer::tuple

#include "tuple.inl"
