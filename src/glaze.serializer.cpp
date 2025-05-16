#include "serializers/glaze/glaze.hpp"

#include <optional>

template <>
struct glz::meta<saucer::serializers::glaze::function_data>
{
    using T                     = saucer::serializers::glaze::function_data;
    static constexpr auto value = object(  //
        "saucer:call", skip{},             //
        "id", &T::id,                      //
        "name", &T::name,                  //
        "params", glz::escaped<&T::params> //
    );
};

template <>
struct glz::meta<saucer::serializers::glaze::result_data>
{
    using T                     = saucer::serializers::glaze::result_data;
    static constexpr auto value = object(  //
        "saucer:resolve", skip{},          //
        "id", &T::id,                      //
        "result", glz::escaped<&T::result> //
    );
};

namespace saucer::serializers::glaze
{
    static constexpr auto opts = glz::opts{
        .error_on_unknown_keys = true,
        .error_on_missing_keys = true,
        .raw_string            = false,
    };

    serializer::~serializer() = default;

    std::string serializer::script() const
    {
        return {};
    }

    std::string serializer::js_serializer() const
    {
        return "JSON.stringify";
    }

    template <typename T>
    std::optional<T> parse_as(std::string_view buffer)
    {
        T value{};

        if (auto err = glz::read<opts>(value, buffer); err)
        {
            return std::nullopt;
        }

        return value;
    }

    serializer::parse_result serializer::parse(std::string_view data) const
    {
        if (auto res = parse_as<function_data>(data); res.has_value())
        {
            return std::make_unique<function_data>(res.value());
        }

        if (auto res = parse_as<result_data>(data); res.has_value())
        {
            return std::make_unique<result_data>(res.value());
        }

        return std::monostate{};
    }
} // namespace saucer::serializers::glaze
