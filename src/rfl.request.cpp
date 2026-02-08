#include "request.utils.hpp"

#include <rfl/json.hpp>
#include <rebind/member.hpp>

using namespace saucer::request::utils;

template <std::size_t N>
consteval auto to_array(std::string_view name)
{
    auto rtn  = std::array<char, N + 1>{};
    rtn.at(N) = '\0';

    for (auto i = 0uz; N > i; i++)
    {
        rtn.at(i) = name.at(i);
    }

    return rtn;
}

template <typename T, std::size_t I = 0, typename... State>
consteval auto generate()
{
    constexpr auto members = rebind::members<T>;
    constexpr auto size    = std::tuple_size_v<decltype(members)>;

    if constexpr (I < size)
    {
        constexpr auto current = std::get<I>(members);
        using current_t        = decltype(current)::type;

        constexpr auto raw_name = current.name;
        constexpr auto name     = to_array<raw_name.size()>(raw_name);

        return generate<T, I + 1, State..., rfl::Field<name, current_t>>();
    }
    else
    {
        return std::type_identity<rfl::NamedTuple<rfl::Field<raw_tag<T, true>, bool>, State...>>{};
    }
}

template <typename T, typename Named>
constexpr auto convert(Named &tuple)
{
    auto unpack = [&]<auto... Is>(std::index_sequence<Is...>)
    {
        return T{tuple.template get<Is + 1>()...};
    };

    return unpack(std::make_index_sequence<rebind::arity<T>>());
}

template <typename T, typename U>
struct variant_index;

template <typename T, typename... Ts>
struct variant_index<T, std::variant<Ts...>>
{
    static constexpr auto value = std::variant<std::type_identity<Ts>...>{std::type_identity<T>{}}.index();
};

template <typename T, typename U>
static constexpr auto variant_index_v = variant_index<T, U>::value;

template <typename T>
struct mapped_variant;

template <typename... Ts>
struct mapped_variant<std::variant<Ts...>>
{
    using type = std::variant<typename decltype(generate<Ts>())::type...>;
};

template <typename T>
using mapped_variant_t = mapped_variant<T>::type;

namespace saucer
{
    std::optional<request::request> request::parse(std::string_view data)
    {
        using variant = mapped_variant_t<request>;
        auto result   = rfl::json::read<variant>(data);

        if (!result.has_value())
        {
            return std::nullopt;
        }

        auto visitor = []<typename T>(T &named) -> request
        {
            using original = std::variant_alternative_t<variant_index_v<T, variant>, request>;
            return convert<original>(named);
        };

        return std::visit(visitor, *result);
    }
} // namespace saucer
