#include "request.utils.hpp"

#include <rfl/json.hpp>
#include <rebind/member.hpp>

using namespace saucer::request::utils;

template <std::size_t N>
consteval auto to_array(std::string_view name)
{
    std::array<char, N + 1> rtn{0};

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

        using type          = decltype(current)::type;
        constexpr auto name = to_array<current.name.size()>(current.name);
        return generate<T, I + 1, State..., rfl::Field<name, type>>();
    }
    else
    {
        return std::type_identity<rfl::NamedTuple<rfl::Field<raw_tag<T, true>, bool>, State...>>{};
    }
}

template <typename T, typename Variant, std::size_t I = 0>
consteval auto index_of()
{
    if constexpr (std::same_as<T, std::variant_alternative_t<I, Variant>>)
    {
        return I;
    }
    else
    {
        return index_of<T, Variant, I + 1>();
    }
}

template <typename T, typename Named>
constexpr auto convert(Named &&tuple)
{
    constexpr auto size = std::tuple_size_v<decltype(rebind::members<T>)>;

    auto unpack = [&]<auto... Is>(std::index_sequence<Is...>)
    {
        return T{std::forward<Named>(tuple).template get<Is + 1>()...};
    };

    return unpack(std::make_index_sequence<size>());
}

template <typename T>
struct variants
{
    using type = void;
};

template <typename... Ts>
struct variants<std::variant<Ts...>>
{
    using type = std::variant<typename decltype(generate<Ts>())::type...>;
};

namespace saucer
{
    std::optional<request::request> request::parse(std::string_view data)
    {
        using variant       = variants<request>;
        using named_variant = variant::type;

        auto result = rfl::json::read<named_variant>(data);

        if (!result)
        {
            return std::nullopt;
        }

        auto visitor = []<typename T>(T &named) -> request
        {
            constexpr auto index = index_of<T, named_variant>();
            using mapped         = std::variant_alternative_t<index, request>;
            return convert<mapped>(named);
        };

        return std::visit(visitor, result.value());
    }
} // namespace saucer
