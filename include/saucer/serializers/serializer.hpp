#pragma once
#include <tuple>
#include <string>
#include <memory>
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

    struct serializer
    {
        enum class error
        {
            argument_count_mismatch,
            type_mismatch,
        };

      public:
        virtual ~serializer() = default;

      public:
        [[nodiscard]] virtual std::string init_script() const = 0;
        [[nodiscard]] virtual std::string js_serializer() const = 0;

      public:
        [[nodiscard]] virtual std::shared_ptr<message_data> parse(const std::string &) const = 0;
    };

    template <typename... T> struct arguments : public std::tuple<T...>
    {
        using std::tuple<T...>::tuple;
    };

    template <typename T> struct is_args;
    template <typename T> using args_t = typename is_args<T>::args_t;
    template <typename T> constexpr bool is_args_v = is_args<T>::value;

    template <typename... T> auto make_arguments(T &&...);
} // namespace saucer

#include "serializer.inl"