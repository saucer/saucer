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
        concept Expected = is_expected<std::remove_cvref_t<T>>::value;
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

    error::domain *unknown_domain();
    error::domain *platform_domain();
    error::domain *contract_domain();
    error::domain *serializer_domain();

    enum class contract_error : std::uint8_t
    {
        instance_exists  = 1,
        required_invalid = 2,
        broken_promise   = 3,
    };

    enum class serializer_error : std::uint8_t
    {
        parsing_failed     = 1,
        signature_mismatch = 2,
    };

    template <>
    struct error::of<contract_error>
    {
        static error operator()(contract_error);
    };

    template <>
    struct error::of<serializer_error>
    {
        static error operator()(serializer_error, std::string);
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

    template <typename T, typename... Ts>
    struct err;
} // namespace saucer

#include "error.inl"
