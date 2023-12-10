#include "cfg.hpp"

#include <saucer/serializers/glaze/glaze.hpp>

using namespace boost::ut;
using namespace boost::ut::literals;

struct a_struct
{
};

suite serializer_suite = []
{
    namespace detail = saucer::serializers::detail::glaze;

    expect(detail::type_name<int>() == "int");
    expect(detail::type_name<float>() == "float");
    expect(detail::type_name<a_struct>().ends_with("a_struct"));
};
