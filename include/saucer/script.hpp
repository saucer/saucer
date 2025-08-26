#pragma once

#include <string>
#include <cstdint>

namespace saucer
{
    struct script
    {
        enum class time : std::uint8_t
        {
            creation,
            ready,
        };

      public:
        std::string code;

      public:
        time run_at;
        bool no_frames{true};

      public:
        bool clearable{true};

      public:
        bool operator==(const script &) const = default;
    };
} // namespace saucer
