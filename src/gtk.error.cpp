#include "gtk.error.hpp"

namespace saucer
{
    error error::of<utils::g_error_ptr>::operator()(const utils::g_error_ptr &error)
    {
        return {.code = error.get()->code, .message = error.get()->message, .kind = g_error_domain()};
    }

    std::string g_error_domain_t::name() const
    {
        return "g_error";
    }

    error::domain *g_error_domain()
    {
        static auto rtn = g_error_domain_t{};
        return &rtn;
    }
} // namespace saucer
