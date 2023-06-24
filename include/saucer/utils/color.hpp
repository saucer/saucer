#pragma once

namespace saucer
{
    struct color
    {
        int r, g, b, a;

      public:
        bool operator==(const color &) const;
    };
} // namespace saucer