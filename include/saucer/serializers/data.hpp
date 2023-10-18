#pragma once
#include <string>
#include <cstdint>

namespace saucer
{
    struct message_data
    {
        virtual ~message_data() = default;
    };

    struct function_data : public message_data
    {
        std::uint64_t id;
        std::string name;
    };

    struct result_data : public message_data
    {
        std::uint64_t id;
    };
} // namespace saucer
