#pragma once

#include <span>
#include <vector>
#include <variant>

namespace saucer
{
    template <typename T>
    class stash
    {
        using owning_t  = std::vector<std::remove_const_t<T>>;
        using viewing_t = std::span<T>;
        using variant_t = std::variant<viewing_t, owning_t>;

      private:
        variant_t m_data;

      private:
        stash(variant_t);

      public:
        [[nodiscard]] T *data() const;
        [[nodiscard]] std::size_t size() const;

      public:
        [[nodiscard]] static stash from(owning_t data);
        [[nodiscard]] static stash view(viewing_t data);
    };
} // namespace saucer

#include "stash.inl"
