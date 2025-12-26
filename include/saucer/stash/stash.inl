#pragma once

#include "stash.hpp"
#include "../utils/overload.hpp"

#include <optional>
#include <functional>

namespace saucer
{
    template <typename T>
    struct detail::lazy
    {
        using callback_t = std::move_only_function<T()>;

      private:
        callback_t m_callback;
        std::optional<T> m_value;

      public:
        lazy(callback_t callback) : m_callback(std::move(callback)) {}

      public:
        template <typename Self>
            requires std::is_lvalue_reference_v<Self>
        [[nodiscard]] auto &value(this Self &&self)
        {
            if (!self.m_value.has_value())
            {
                self.m_value.emplace(self.m_callback());
            }

            return std::forward<Self>(self).m_value.value();
        }
    };

    template <typename T>
    basic_stash<T>::basic_stash(variant_t data) : m_data(std::move(data))
    {
    }

    template <typename T>
    const T *basic_stash<T>::data() const
    {
        auto visitor = overload{
            [](const lazy_t &data) { return data->value().data(); },
            [](const auto &data) { return data.data(); },
        };

        return std::visit(visitor, m_data);
    }

    template <typename T>
    std::size_t basic_stash<T>::size() const
    {
        auto visitor = overload{
            [](const lazy_t &data) { return data->value().size(); },
            [](const auto &data) { return data.size(); },
        };

        return std::visit(visitor, m_data);
    }

    template <typename T>
    std::string basic_stash<T>::str()
        requires std::same_as<T, std::uint8_t>
    {
        const auto *begin = reinterpret_cast<const char *>(data());
        const auto *end   = reinterpret_cast<const char *>(begin + size());

        return {begin, end};
    }

    template <typename T>
    basic_stash<T> basic_stash<T>::from(owning_t data)
    {
        return {std::move(data)};
    }

    template <typename T>
    basic_stash<T> basic_stash<T>::view(viewing_t data)
    {
        return {std::move(data)};
    }

    template <typename T>
    basic_stash<T> basic_stash<T>::lazy(lazy_t data)
    {
        return {std::move(data)};
    }

    template <typename T>
    basic_stash<T> basic_stash<T>::from_str(std::string_view data)
        requires std::same_as<T, std::uint8_t>
    {
        auto *const begin = reinterpret_cast<const T *>(data.data());
        auto *const end   = reinterpret_cast<const T *>(data.data() + data.size());

        return {owning_t{begin, end}};
    }

    template <typename T>
    basic_stash<T> basic_stash<T>::view_str(std::string_view data)
        requires std::same_as<T, std::uint8_t>
    {
        auto *const begin = reinterpret_cast<const T *>(data.data());
        auto *const end   = reinterpret_cast<const T *>(data.data() + data.size());

        return {viewing_t{begin, end}};
    }

    template <typename T>
    template <typename Callback>
        requires std::same_as<std::invoke_result_t<Callback>, basic_stash<T>>
    basic_stash<T> basic_stash<T>::lazy(Callback callback)
    {
        return {std::make_shared<detail::lazy<basic_stash<T>>>(std::move(callback))};
    }

    template <typename T>
    basic_stash<T> basic_stash<T>::empty()
    {
        return {{}};
    }
} // namespace saucer
