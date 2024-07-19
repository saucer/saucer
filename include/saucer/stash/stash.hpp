#pragma once

#include <span>
#include <vector>

#include <future>
#include <variant>

namespace saucer
{
    template <typename T>
    class stash
    {
        using owning_t  = std::vector<std::remove_const_t<T>>;
        using viewing_t = std::span<T>;

      public:
        using data_t    = std::variant<viewing_t, owning_t>;
        using lazy_t    = std::shared_future<data_t>;
        using variant_t = std::variant<data_t, lazy_t>;

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

      public:
        [[nodiscard]] static stash lazy(lazy_t data);
        template <typename Callback>
        [[nodiscard]] static stash lazy(Callback &&);
    };
} // namespace saucer

#include "stash.inl"
