#pragma once

#include "error.impl.hpp"
#include "gtk.utils.hpp"

namespace saucer
{
    template <>
    struct error::of<utils::g_error_ptr>
    {
        static error operator()(const utils::g_error_ptr &);
    };

    struct g_error_domain_t : error::domain
    {
        [[nodiscard]] std::string name() const override;
    };

    error::domain *g_error_domain();
} // namespace saucer
