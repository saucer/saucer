#include "serializers/glaze/glaze.hpp"

#include <expected>

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
    std::expected<T, glz::error_ctx> parse_as(const std::string &buffer)
    {
        static constexpr auto opts = glz::opts{
            .error_on_unknown_keys = true,
            .error_on_missing_keys = true,
            .raw_string            = false,
        };

        T value{};
        auto error = glz::read<opts>(value, buffer);

        if (error)
        {
            return std::unexpected{error};
        }

        return value;
    }

    serializer::parse_result serializer::parse(const std::string &data) const
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
