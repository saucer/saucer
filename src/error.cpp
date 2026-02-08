#include "error.impl.hpp"

namespace saucer
{
    std::string name_of(contract_error error)
    {
        switch (error)
        {
            using enum contract_error;

        case instance_exists:
            return "Instance already exists";
        case required_invalid:
            return "Required value invalid";
        case broken_promise:
            return "Broken Promise";
        }
    }

    error error::of<contract_error>::operator()(contract_error error)
    {
        return {.code = static_cast<int>(error), .message = name_of(error), .kind = contract_domain()};
    }

    error error::of<serializer_error>::operator()(serializer_error error, std::string message)
    {
        return {.code = static_cast<int>(error), .message = std::move(message), .kind = serializer_domain()};
    }

    error error::of<std::errc>::operator()(std::errc error)
    {
        return error::of<std::error_code>{}(std::make_error_code(error));
    }

    error error::of<std::error_code>::operator()(std::error_code error)
    {
        return {.code = error.value(), .message = error.message(), .kind = platform_domain()};
    }

    std::string contract_domain_t::name() const
    {
        return "saucer::contract";
    }

    std::string platform_domain_t::name() const
    {
        return std::generic_category().name();
    }

    std::string unknown_domain_t::name() const
    {
        return "unknown";
    }

    std::string serializer_domain_t::name() const
    {
        return "saucer::serializer";
    }

    error::domain *contract_domain()
    {
        static auto rtn = contract_domain_t{};
        return &rtn;
    }

    error::domain *platform_domain()
    {
        static auto rtn = platform_domain_t{};
        return &rtn;
    }

    error::domain *unknown_domain()
    {
        static auto rtn = unknown_domain_t{};
        return &rtn;
    }

    error::domain *serializer_domain()
    {
        static auto rtn = serializer_domain_t{};
        return &rtn;
    }
} // namespace saucer
