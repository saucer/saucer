#pragma once

#include <memory>
#include <cstdint>

#include <variant>
#include <string_view>

#include <span>
#include <vector>

namespace saucer
{
    namespace detail
    {
        template <typename T>
        struct lazy;
    };

    template <typename T = std::uint8_t>
    struct stash
    {
        using owning_t  = std::vector<std::remove_const_t<T>>;
        using viewing_t = std::span<std::add_const_t<T>>;
        using lazy_t    = std::shared_ptr<detail::lazy<stash<T>>>;
        using variant_t = std::variant<owning_t, viewing_t, lazy_t>;

      private:
        variant_t m_data;

      private:
        stash(variant_t);

      public:
        [[nodiscard]] const T *data() const;
        [[nodiscard]] std::size_t size() const;

      public:
        [[nodiscard]] static stash from(owning_t data);
        [[nodiscard]] static stash view(viewing_t data);
        [[nodiscard]] static stash lazy(lazy_t data);

      public:
        [[nodiscard]] static stash view(std::string_view data)
            requires std::same_as<T, std::uint8_t>;

      public:
        template <typename Callback>
            requires std::same_as<std::invoke_result_t<Callback>, stash<T>>
        [[nodiscard]] static stash lazy(Callback);

      public:
        [[nodiscard]] static stash empty();
    };
} // namespace saucer

#include "stash.inl"
