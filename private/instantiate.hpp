#pragma once

#include <boost/preprocessor.hpp>

#define SAUCER_INSTANTIATE_IMPL(_, N, DATA) BOOST_PP_TUPLE_ELEM(0, DATA)(N, BOOST_PP_TUPLE_ELEM(1, DATA))
#define SAUCER_INSTANTIATE(COUNT, MACRO, DATA) BOOST_PP_REPEAT(COUNT, SAUCER_INSTANTIATE_IMPL, (MACRO, DATA))

#define SAUCER_INSTANTIATE_EVENTS_IMPL(N, DATA)                                                                             \
    template std::uint64_t BOOST_PP_TUPLE_ELEM(0, DATA)::on<static_cast<BOOST_PP_TUPLE_ELEM(1, DATA)>(N)>(                  \
        events::type<static_cast<BOOST_PP_TUPLE_ELEM(1, DATA)>(N)>);                                                        \
    template void BOOST_PP_TUPLE_ELEM(0, DATA)::once<static_cast<BOOST_PP_TUPLE_ELEM(1, DATA)>(N)>(                         \
        events::type<static_cast<BOOST_PP_TUPLE_ELEM(1, DATA)>(N)>);

#define SAUCER_INSTANTIATE_EVENTS(COUNT, CLASS, ENUM)                                                                       \
    SAUCER_INSTANTIATE(COUNT, SAUCER_INSTANTIATE_EVENTS_IMPL, (CLASS, ENUM))
