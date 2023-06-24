#include "utils/color.hpp"

namespace saucer
{
    bool color::operator==(const color &other) const
    {
        return other.r == r && other.g == g && other.b == b && other.a == a;
    }
} // namespace saucer