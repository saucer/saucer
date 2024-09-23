#pragma once

#include <string>

namespace saucer
{
    enum class load_time
    {
        creation,
        ready,
    };

    enum class web_frame
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
        bool permanent{false};

      public:
        bool operator==(const script &) const = default;
    };
} // namespace saucer
