#include "utils.win32.hpp"

#include <Windows.h>

namespace saucer
{
    std::string last_error()
    {
        auto error = GetLastError();

        if (!error)
        {
            return "<No Error>";
        }

        constexpr DWORD dw_flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | //
                                   FORMAT_MESSAGE_FROM_SYSTEM |     //
                                   FORMAT_MESSAGE_IGNORE_INSERTS;

        LPWSTR buffer{};
        auto lang_id = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
        auto size = FormatMessageW(dw_flags, nullptr, error, lang_id, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);

        auto message = narrow(std::wstring{buffer, size});
        LocalFree(buffer);

        return message;
    }

    std::wstring widen(const std::string &narrow)
    {
        auto size = MultiByteToWideChar(65001, 0, narrow.c_str(), -1, nullptr, 0);

        if (!size)
        {
            return {};
        }

        std::wstring out(size, 0);
        MultiByteToWideChar(65001, 0, narrow.c_str(), -1, out.data(), size);

        out.resize(size - 1);
        return out;
    }

    std::string narrow(const std::wstring &wide)
    {
        auto size =
            WideCharToMultiByte(65001, 0, wide.c_str(), static_cast<int>(wide.length()), nullptr, 0, nullptr, nullptr);

        if (!size)
        {
            return {};
        }

        std::string out(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, out.data(), size, nullptr, nullptr);

        return out;
    }
} // namespace saucer