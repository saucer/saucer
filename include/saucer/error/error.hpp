#pragma once

#include <string>
#include <cstdint>

#include <expected>
#include <source_location>

#include <system_error>

namespace saucer
{
    namespace detail
    {
        template <typename T>
        struct is_expected;

        template <typename T>
        concept Expected = is_expected<T>::value;
    } // namespace detail

    struct error
    {
        template <typename T>
        struct of;
        struct domain;

      public:
        int code;
        std::string message;

      public:
        const domain *kind;
        std::source_location location;
    };

    struct error::domain
    {
        virtual ~domain()                              = default;
        [[nodiscard]] virtual std::string name() const = 0;
    };

    error::domain *contract_domain();
    error::domain *unknown_domain();
    error::domain *platform_domain();

    enum class contract_error : std::uint8_t
    {
        instance_exists  = 1,
        required_invalid = 2,
        broken_promise   = 3,
    };

    template <>
    struct error::of<contract_error>
    {
        static error operator()(contract_error);
    };

    template <>
    struct error::of<std::errc>
    {
        static error operator()(std::errc);
    };

    template <>
    struct error::of<std::error_code>
    {
        static error operator()(std::error_code);
    };

    template <typename T = void>
    using result = std::expected<T, error>;

    inline auto err(error);

    template <typename T>
        requires(detail::Expected<std::remove_cvref_t<T>>)
    auto err(T &&);

    template <typename T>
        requires(not detail::Expected<std::remove_cvref_t<T>>)
    auto err(T &&, std::source_location = std::source_location::current());
} // namespace saucer

#include "error.inl"
