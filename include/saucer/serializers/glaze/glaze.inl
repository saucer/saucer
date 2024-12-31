#pragma once

#include "glaze.hpp"

#include <fmt/ranges.h>

#include <rebind/name.hpp>
#include <rebind/utils/enum.hpp>

namespace saucer::serializers::glaze
{
    namespace impl
    {
        static constexpr auto opts = glz::opts{.error_on_missing_keys = true};

        template <typename T>
        concept Serializable = glz::read_supported<opts.format, T>;

        template <typename T>
        auto can_parse(T &value, const glz::json_t &data)
        {
            return !glz::read<opts>(value, data);
        }

        template <typename T>
            requires(not tuple::Tuple<T>)
        std::optional<std::string> mismatch(T &value, const glz::json_t &data)
        {
            if (can_parse(value, data))
            {
                return std::nullopt;
            }

            return fmt::format("Expected value of type '{}'", rebind::type_name<T>);
        }

        template <tuple::Tuple T, std::size_t I = 0>
        std::optional<std::string> mismatch(T &value, const glz::json_t &data)
        {
            if constexpr (I < std::tuple_size_v<T>)
            {
                using current = std::tuple_element_t<I, T>;

                if (can_parse(std::get<I>(value), data[I]))
                {
                    return mismatch<T, I + 1>(value, data);
                }

                return fmt::format("Expected parameter {} to be of type '{}'", I, rebind::type_name<current>);
            }
            else
            {
                return std::nullopt;
            }
        }
    } // namespace impl

    template <typename T>
    interface::result<T> interface::parse(const std::string &data)
    {
        static_assert(impl::Serializable<T>, "T should be serializable");

        T rtn{};

        if (auto err = glz::read<impl::opts>(rtn, data); !err)
        {
            return rtn;
        }

        glz::json_t json{};

        if (auto err = glz::read<impl::opts>(json, data); err)
        {
            const auto name = rebind::utils::find_enum_name(err.ec);
            return std::unexpected{std::string{name.value_or("<Unknown Parsing Error>")}};
        }

        return std::unexpected{impl::mismatch(rtn, json).value_or("<Unknown Mismatch>")};
    }

    template <typename T>
    interface::result<T> interface::parse(const result_data &data)
    {
        return parse<T>(data.result.str);
    }

    template <typename T>
    interface::result<T> interface::parse(const function_data &data)
    {
        return parse<T>(data.params.str);
    }

    template <typename T>
    std::string interface::serialize(T &&value)
    {
        static_assert(impl::Serializable<T>, "T should be serializable");
        return glz::write<impl::opts>(std::forward<T>(value)).value_or("null");
    }
} // namespace saucer::serializers::glaze
