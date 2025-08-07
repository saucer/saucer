#include "error/error.hpp"

#include <utility>

struct category_t : std::error_category
{
    [[nodiscard]] const char *name() const noexcept override
    {
        return "contract";
    }

    [[nodiscard]] std::string message(int value) const override
    {
        switch (static_cast<saucer::contract_error>(value))
        {
            using enum saucer::contract_error;

        case success:
            return "success";

        case instance_exists:
            return "instance already exists";

        case not_main_thread:
            return "not called from main thread";

        case required_invalid:
            return "required argument was invalid";
        }

        std::unreachable();
    }
} const category;

const std::error_category &saucer::contract_category()
{
    return category;
}

std::error_code saucer::make_error_code(contract_error error)
{
    return {static_cast<int>(error), contract_category()};
}
