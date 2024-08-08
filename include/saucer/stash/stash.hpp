#pragma once

#include <memory>
#include <ranges>

#include <cstddef>

#include <span>
#include <vector>

#include <future>
#include <variant>

namespace saucer
{
    template <typename T = std::uint8_t>
    class stash
    {
        using owning_t  = std::vector<std::remove_const_t<T>>;
        using viewing_t = std::span<std::add_const_t<T>>;
        using lazy_t    = std::shared_future<std::shared_ptr<stash<T>>>;
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

      public:
        [[nodiscard]] static stash lazy(lazy_t data);
        template <typename Callback>
        [[nodiscard]] static stash lazy(Callback);

      public:
        [[nodiscard]] static stash empty();
    };

    template <typename T = std::uint8_t, typename V>
        requires std::ranges::range<V>
    auto make_stash(const V &data);
} // namespace saucer

#include "stash.inl"
