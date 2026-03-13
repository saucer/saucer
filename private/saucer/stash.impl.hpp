#pragma once

#include <saucer/stash.hpp>
#include <saucer/error/error.hpp>

#include <variant>

namespace saucer
{
    struct stash::impl
    {
        struct lazy;
        struct view;
        struct owng;

      public:
        virtual ~impl() = default;

      public:
        [[nodiscard]] virtual span data() const = 0;

      public:
        [[nodiscard]] virtual std::size_t type() const            = 0;
        [[nodiscard]] virtual std::unique_ptr<impl> clone() const = 0;

      public:
        template <typename T>
        [[nodiscard]] static std::size_t id_of();
        [[nodiscard]] static std::size_t count();
    };

    struct stash::impl::lazy : stash::impl
    {
        mutable std::variant<std::function<stash()>, stash> m_data;

      public:
        lazy(std::function<stash()>);

      private:
        [[nodiscard]] stash &unwrap() const;

      public:
        [[nodiscard]] span data() const override;

      public:
        [[nodiscard]] std::size_t type() const override;
        [[nodiscard]] std::unique_ptr<impl> clone() const override;
    };

    struct stash::impl::view : stash::impl
    {
        span m_data;

      public:
        view(span);

      public:
        [[nodiscard]] span data() const override;

      public:
        [[nodiscard]] std::size_t type() const override;
        [[nodiscard]] std::unique_ptr<impl> clone() const override;
    };

    struct stash::impl::owng : stash::impl
    {
        vec m_data;

      public:
        owng(vec);

      public:
        [[nodiscard]] span data() const override;

      public:
        [[nodiscard]] std::size_t type() const override;
        [[nodiscard]] std::unique_ptr<impl> clone() const override;
    };
} // namespace saucer

#include "stash.impl.inl"
