#pragma once

#include <string>
#include <cstdint>

namespace saucer
{
    enum class load_time : std::uint8_t
    {
        creation,
        ready,
    };

    enum class web_frame : std::uint8_t
    {
        top,
        all,
    };

    struct script
    {
        std::string code;

      public:
        load_time time;
        web_frame frame{web_frame::top};

      public:
        bool clearable{true};

      public:
        bool operator==(const script &) const = default;
    };
} // namespace saucer
