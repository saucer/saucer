#pragma once

#include "webview.hpp"
#include "utils/traits.hpp"

namespace saucer
{
    template <typename T>
    void webview::handle_scheme(const std::string &name, T &&handler, launch policy)
    {
        using converter = traits::converter<T, std::tuple<scheme::request>, scheme::executor>;
        handle_scheme(name, scheme::resolver{converter::convert(std::forward<T>(handler))}, policy);
    }
} // namespace saucer
