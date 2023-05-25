#pragma once
#include <string>
#include <cstdint>

namespace saucer
{
    struct message_data
    {
        std::size_t id;
        virtual ~message_data() = default;
    };

    struct function_data : public message_data
    {
        std::string function;
        ~function_data() override = default;
    };

    struct result_data : public message_data
    {
        ~result_data() override = default;
    };
} // namespace saucer