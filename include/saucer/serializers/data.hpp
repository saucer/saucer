#pragma once

#include <string>
#include <memory>

#include <variant>
#include <cstdint>

namespace saucer
{
    struct function_data
    {
        std::uint64_t id;
        std::string name;
    };

    struct result_data
    {
        std::uint64_t id;
    };

    using message_data = std::variant<std::unique_ptr<function_data>, std::unique_ptr<result_data>, std::monostate>;
} // namespace saucer
