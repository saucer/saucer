#pragma once

#include "glaze.hpp"

#include <rebind/name.hpp>
#include <rebind/utils/enum.hpp>

namespace saucer::serializers::glaze
{
    namespace detail
    {
        static constexpr auto opts = glz::opts{.error_on_missing_keys = true};

        template <typename T>
        auto try_parse(T &value, const glz::generic &data)
        {
            return !glz::read<opts>(value, data);
        }

        template <typename T>
            requires(not tuple::Tuple<T>)
        std::optional<std::string> mismatch(T &value, const glz::generic &data)
        {
            if (try_parse(value, data))
            {
                return std::nullopt;
            }

            return std::format("Expected value of type '{}'", rebind::type_name<T>);
        }

        template <tuple::Tuple T, std::size_t I = 0>
            requires(I >= std::tuple_size_v<T>)
        std::optional<std::string> mismatch(T &, const glz::generic &)
        {
            return std::nullopt;
        }

        template <tuple::Tuple T, std::size_t I = 0>
        std::optional<std::string> mismatch(T &value, const glz::generic &data)
        {
            using current = std::tuple_element_t<I, T>;

            if (try_parse(std::get<I>(value), data[I]))
            {
                return mismatch<T, I + 1>(value, data);
            }

            return std::format("Expected parameter {} to be of type '{}'", I, rebind::type_name<current>);
        }
    } // namespace detail

    template <Writable T>
    std::string serializer::write(T &&value)
    {
        return glz::write<detail::opts>(std::forward<T>(value)).value_or("null");
    }

    template <Readable T>
    serializer::result<T> serializer::read(std::string_view data)
    {
        T rtn{};

        if (auto err = glz::read<detail::opts>(rtn, data); !err)
        {
            return rtn;
        }

        glz::generic json{};

        if (auto err = glz::read<detail::opts>(json, data); err)
        {
            const auto name = rebind::utils::find_enum_name(err.ec).value_or("Unknown");
            return std::unexpected{std::string{name}};
        }

        return std::unexpected{detail::mismatch(rtn, json).value_or("Unknown Mismatch")};
    }

    template <Readable T>
    serializer::result<T> serializer::read(const result_data &data)
    {
        return read<T>(data.result.str);
    }

    template <Readable T>
    serializer::result<T> serializer::read(const function_data &data)
    {
        return read<T>(data.params.str);
    }
} // namespace saucer::serializers::glaze
