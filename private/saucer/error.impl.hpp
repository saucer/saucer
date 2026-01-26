#pragma once

#include <saucer/error/error.hpp>

namespace saucer
{
    struct contract_domain_t : error::domain
    {
        [[nodiscard]] std::string name() const override;
    };

    struct platform_domain_t : error::domain
    {
        [[nodiscard]] std::string name() const override;
    };

    struct unknown_domain_t : error::domain
    {
        [[nodiscard]] std::string name() const override;
    };

    struct serializer_domain_t : error::domain
    {
        [[nodiscard]] std::string name() const override;
    };

    template <typename T>
    concept Unwrappable = requires(T value) {
        { value.has_value() } -> std::same_as<bool>;
        requires std::default_initializable<typename std::remove_cvref_t<T>::value_type>;
    };

    template <Unwrappable T>
    auto unwrap_safe(T &&value);
} // namespace saucer

#include "error.impl.inl"
