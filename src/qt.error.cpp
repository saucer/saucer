#include "error/qt.hpp"

#include <utility>

struct category_t : std::error_category
{
    [[nodiscard]] const char *name() const noexcept override
    {
        return "qt";
    }

    [[nodiscard]] std::string message(int value) const override
    {
        switch (static_cast<saucer::qt_error>(value))
        {
            using enum saucer::qt_error;

        case success:
            return "success";

        case no_web_channel:
            return "no web-channel";
        }

        std::unreachable();
    }
} const category;

std::error_code saucer::make_error_code(qt_error error)
{
    return {static_cast<int>(error), category};
}
