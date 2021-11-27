#pragma once
#include <Windows.h>
#include <string>

namespace saucer
{
    namespace utils
    {
        inline std::string narrow(const std::wstring &w_str)
        {
            auto sz = WideCharToMultiByte(65001, 0, w_str.c_str(), static_cast<int>(w_str.length()), nullptr, 0, nullptr, nullptr);
            if (!sz)
            {
                return {};
            }

            std::string out(sz, 0);
            WideCharToMultiByte(CP_UTF8, 0, w_str.c_str(), -1, out.data(), sz, nullptr, nullptr);
            return out;
        }

        inline std::wstring widen(const std::string &str)
        {
            auto wsz = MultiByteToWideChar(65001, 0, str.c_str(), -1, nullptr, 0);
            if (!wsz)
            {
                return {};
            }

            std::wstring out(wsz, 0);
            MultiByteToWideChar(65001, 0, str.c_str(), -1, out.data(), wsz);
            out.resize(wsz - 1);
            return out;
        }
    } // namespace utils
} // namespace saucer