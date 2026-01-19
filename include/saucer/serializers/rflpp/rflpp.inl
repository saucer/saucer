#pragma once

#include "rflpp.hpp"

namespace saucer::serializers::rflpp
{
    namespace detail
    {
        template <typename T>
        struct is_fixed_string : std::false_type
        {
        };

        template <std::size_t N>
        struct is_fixed_string<const char (&)[N]> : std::true_type
        {
        };

        template <typename T>
        concept FixedString = is_fixed_string<T>::value;

        template <typename T>
        std::string write(T &&value)
        {
            return rfl::json::write(std::forward<T>(value));
        }

        template <FixedString T>
        std::string write(T &&value)
        {
            return write(std::string{std::forward<T>(value)});
        }

        template <typename T, typename U>
        auto read_impl(U &&value)
        {
            return rfl::json::read<T>(std::forward<U>(value));
        }

        template <typename T>
        auto read_impl(const rfl::Generic &value)
        {
            return rfl::from_generic<T>(value);
        }

        template <typename T, typename U>
        serializer::result<T> read(U &&value)
        {
            auto rtn = read_impl<T>(std::forward<U>(value));

            if (!rtn.has_value())
            {
                return std::unexpected{rtn.error().what()};
            }

            return *rtn;
        }
    } // namespace detail

    template <Writable T>
    std::string serializer::write(T &&value)
    {
        return detail::write(std::forward<T>(value));
    }

    template <Readable T>
    serializer::result<T> serializer::read(std::string_view value)
    {
        return detail::read<T>(value);
    }

    template <Readable T>
    serializer::result<T> serializer::read(const result_data &data)
    {
        return detail::read<T>(data.result);
    }

    template <Readable T>
    serializer::result<T> serializer::read(const function_data &data)
    {
        return detail::read<T>(data.params);
    }
} // namespace saucer::serializers::rflpp
