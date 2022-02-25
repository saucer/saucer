#pragma once
#include <string>
#include <memory>
#include <functional>

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

    struct variable_data : public message_data
    {
        std::string variable;
        ~variable_data() override = default;
    };

    struct serializer
    {
        enum class error
        {
            argument_count_mismatch,
            type_mismatch,
        };

        virtual ~serializer() = default;
        virtual std::string initialization_script() const = 0;
        virtual std::string java_script_serializer() const = 0;
        virtual std::shared_ptr<message_data> parse(const std::string &) const = 0;
    };

    template <typename... T> class arguments : public std::tuple<T...>
    {
      public:
        using std::tuple<T...>::tuple;
    };

    template <typename... T> auto make_arguments(T &&...);
} // namespace saucer

#include "serializer.inl"