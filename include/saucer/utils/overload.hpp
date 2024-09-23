#pragma once

namespace saucer
{
    template <typename... Ts>
    struct overload : Ts...
    {
        using Ts::operator()...;
    };

#ifdef __APPLE__
    template <class... Ts>
    overload(Ts...) -> overload<Ts...>;
#endif
} // namespace saucer
