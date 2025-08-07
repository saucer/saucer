#include "error/creation.hpp"

#include <utility>

struct category_t : std::error_category
{
    [[nodiscard]] const char *name() const noexcept override
    {
        return "creation";
    }

    [[nodiscard]] std::string message(int value) const override
    {
        switch (static_cast<saucer::creation_error>(value))
        {
            using enum saucer::creation_error;

        case success:
            return "success";

        case bad_argument:
            return "bad argument";

        case exists_already:
            return "exists already";

        case not_thread_safe:
            return "not thread-safe";
        }

        std::unreachable();
    }
} const category;

std::error_code saucer::make_error_code(creation_error error)
{
    return {static_cast<int>(error), category};
}
