#pragma once
#include <array>
#include <cstdint>

namespace saucer::embedded
{
    inline constexpr std::array<std::uint8_t, 150> src_index_js = {
        0x63, 0x6f, 0x6e, 0x73, 0x74, 0x20, 0x65, 0x6c, 0x65, 0x6d, 0x65, 0x6e, 0x74, 0x20, 0x3d, 0x20, 0x64,
        0x6f, 0x63, 0x75, 0x6d, 0x65, 0x6e, 0x74, 0x2e, 0x63, 0x72, 0x65, 0x61, 0x74, 0x65, 0x45, 0x6c, 0x65,
        0x6d, 0x65, 0x6e, 0x74, 0x28, 0x27, 0x70, 0x27, 0x29, 0x3b, 0xa,  0x65, 0x6c, 0x65, 0x6d, 0x65, 0x6e,
        0x74, 0x2e, 0x69, 0x6e, 0x6e, 0x65, 0x72, 0x54, 0x65, 0x78, 0x74, 0x20, 0x3d, 0x20, 0x22, 0x48, 0x65,
        0x6c, 0x6c, 0x6f, 0x20, 0x66, 0x72, 0x6f, 0x6d, 0x20, 0x61, 0x75, 0x74, 0x6f, 0x6d, 0x61, 0x74, 0x65,
        0x64, 0x20, 0x65, 0x6d, 0x62, 0x65, 0x64, 0x64, 0x69, 0x6e, 0x67, 0x20, 0x65, 0x78, 0x61, 0x6d, 0x70,
        0x6c, 0x65, 0x73, 0x20, 0x69, 0x6e, 0x64, 0x65, 0x78, 0x2e, 0x6a, 0x73, 0x21, 0x22, 0x3b, 0xa,  0xa,
        0x64, 0x6f, 0x63, 0x75, 0x6d, 0x65, 0x6e, 0x74, 0x2e, 0x62, 0x6f, 0x64, 0x79, 0x2e, 0x61, 0x70, 0x70,
        0x65, 0x6e, 0x64, 0x28, 0x65, 0x6c, 0x65, 0x6d, 0x65, 0x6e, 0x74, 0x29, 0x3b, 0xa};
} // namespace saucer::embedded