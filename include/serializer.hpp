#pragma once
#include <functional>
#include <memory>
#include <string>

namespace saucer
{
    struct message_data
    {
        std::size_t id;
        std::string function;
        virtual ~message_data() = default;
    };

    struct serializer
    {
        enum class error
        {
            argument_mismatch,
            parser_mismatch,
        };

        virtual ~serializer() = default;
        virtual std::string java_script_serializer() const = 0;
        virtual std::shared_ptr<message_data> parse(const std::string &) = 0;
    };
} // namespace saucer