#include "gtk.error.hpp"

namespace saucer
{
    error::of<utils::g_error_ptr>::of(utils::g_error_ptr value) : value(std::move(value)) {}

    error::of<utils::g_error_ptr>::of(const of &other) : of(g_error_copy(other.value.get())) {}

    int error::of<utils::g_error_ptr>::code() const
    {
        return value.get()->code;
    }

    error::category error::of<utils::g_error_ptr>::type() const
    {
        return category::platform;
    }

    std::string error::of<utils::g_error_ptr>::message() const
    {
        return value.get()->message;
    }
} // namespace saucer
