#pragma once

#include <boost/preprocessor/tuple.hpp>
#include <boost/preprocessor/repeat.hpp>

#define INSTANTIATE_IMPL(_, N, DATA)                                                                                   \
    template BOOST_PP_TUPLE_ELEM(2, 1, DATA) BOOST_PP_TUPLE_ELEM(2, 0, DATA)::BOOST_PP_TUPLE_ELEM(                     \
        2, 2, DATA)<static_cast<BOOST_PP_TUPLE_ELEM(2, 3, DATA)>(N)>(                                                  \
        events::type_t<static_cast<BOOST_PP_TUPLE_ELEM(2, 3, DATA)>(N)> &&);

#define INSTANTIATE_EVENTS(CLASS, AMOUNT, ENUM)                                                                        \
    BOOST_PP_REPEAT(AMOUNT, INSTANTIATE_IMPL, (CLASS, std::uint64_t, on, ENUM))                                        \
    BOOST_PP_REPEAT(AMOUNT, INSTANTIATE_IMPL, (CLASS, void, once, ENUM))
