#include "request.utils.hpp"

#include <glaze/glaze.hpp>

using namespace saucer::request::utils;

template <typename T, std::size_t I = 0, typename... State>
consteval auto generate(State... state)
{
    constexpr auto names = glz::member_names<T>;
    constexpr auto size  = names.size();

    if constexpr (I < size)
    {
        return generate<T, I + 1>(state..., names.at(I), [](auto &&self) -> auto & { return glz::get<I>(glz::to_tie(self)); });
    }
    else
    {
        return glz::object(tag<T>, glz::skip{}, state...);
    }
}

template <typename T>
    requires is_request<T>
struct glz::meta<T>
{
    static constexpr auto value = generate<T>();
};

namespace saucer
{
    static constexpr auto opts = glz::opts{.error_on_unknown_keys = true, .error_on_missing_keys = true};

    std::optional<request::request> request::parse(std::string_view data)
    {
        request rtn{};

        if (auto err = glz::read<opts>(rtn, data); err)
        {
            return std::nullopt;
        }

        return rtn;
    }
} // namespace saucer
