#pragma once

#include "rflpp.hpp"

namespace saucer::serializers::rflpp
{
    namespace impl
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
        concept fixed_string = is_fixed_string<T>::value;

        template <typename T>
        concept Readable = requires(std::string data) {
            { rfl::json::read<std::remove_cvref_t<T>>(data) };
        };

        template <typename T>
        concept Writable = requires(T value) {
            { rfl::json::write(value) };
        };

        template <typename T>
        auto parse(const std::string &data)
        {
            return rfl::json::read<T>(data);
        }

        template <typename T>
        auto parse(const rfl::Generic &data)
        {
            return rfl::from_generic<T>(data);
        }
    } // namespace impl

    template <typename T>
    interface::result<T> interface::parse(const auto &data)
    {
        static_assert(impl::Readable<T>, "T should be serializable");

        auto rtn = impl::parse<T>(data);

        if (rtn)
        {
            return rtn.value();
        }

        return std::unexpected{rtn.error().value().what()};
    }

    template <typename T>
    interface::result<T> interface::parse(const result_data &data)
    {
        return parse<T>(data.result);
    }

    template <typename T>
    interface::result<T> interface::parse(const function_data &data)
    {
        return parse<T>(data.params);
    }

    template <typename T>
    std::string interface::serialize(T &&value)
    {
        static_assert(impl::Writable<T>, "T should be serializable");

        if constexpr (impl::fixed_string<T>)
        {
            return rfl::json::write(std::string{std::forward<T>(value)});
        }
        else
        {
            return rfl::json::write(std::forward<T>(value));
        }
    }
} // namespace saucer::serializers::rflpp
