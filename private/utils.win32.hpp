#pragma once
#include <memory>
#include <string>
#include <concepts>

#include <wrl.h>
#include <WebView2.h>

namespace saucer::utils
{
    void set_dpi_awareness();

    [[nodiscard]] std::string error();

    [[nodiscard]] std::string narrow(const std::wstring &wide);
    [[nodiscard]] std::wstring widen(const std::string &narrow);
} // namespace saucer::utils
