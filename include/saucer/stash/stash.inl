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
                self.m_value.emplace(std::invoke(self.m_callback));
            }

            return std::forward<Self>(self).m_value.value();
        }
    };

    template <typename T>
    stash<T>::stash(variant_t data) : m_data(std::move(data))
    {
    }

    template <typename T>
    const T *stash<T>::data() const
    {
        overload visitor = {
            [](const lazy_t &data) { return data->value().data(); },
            [](const auto &data) { return data.data(); },
        };

        return std::visit(visitor, m_data);
    }

    template <typename T>
    std::size_t stash<T>::size() const
    {
        overload visitor = {
            [](const lazy_t &data) { return data->value().size(); },
            [](const auto &data) { return data.size(); },
        };

        return std::visit(visitor, m_data);
    }

    template <typename T>
    stash<T> stash<T>::from(owning_t data)
    {
        return {std::move(data)};
    }

    template <typename T>
    stash<T> stash<T>::view(viewing_t data)
    {
        return {std::move(data)};
    }

    template <typename T>
    stash<T> stash<T>::lazy(lazy_t data)
    {
        return {std::move(data)};
    }

    template <typename T>
    stash<T> stash<T>::view(std::string_view data)
        requires std::same_as<T, std::uint8_t>
    {
        auto *const begin = reinterpret_cast<const T *>(data.begin());
        auto *const end   = reinterpret_cast<const T *>(data.end());

        return {viewing_t{begin, end}};
    }

    template <typename T>
    template <typename Callback>
        requires std::same_as<std::invoke_result_t<Callback>, stash<T>>
    stash<T> stash<T>::lazy(Callback callback)
    {
        return {std::make_shared<detail::lazy<stash<T>>>(std::move(callback))};
    }

    template <typename T>
    stash<T> stash<T>::empty()
    {
        return {{}};
    }
} // namespace saucer
