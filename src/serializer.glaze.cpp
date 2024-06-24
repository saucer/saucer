#include "serializers/glaze/glaze.hpp"

#include <tl/expected.hpp>

template <>
struct glz::meta<saucer::serializers::glaze_function_data>
{
    using T                     = saucer::serializers::glaze_function_data;
    static constexpr auto value = object(  //
        "id", &T::id,                      //
        "name", &T::name,                  //
        "params", glz::escaped<&T::params> //
    );
};

template <>
struct glz::meta<saucer::serializers::glaze_result_data>
{
    using T                     = saucer::serializers::glaze_result_data;
    static constexpr auto value = object(  //
        "id", &T::id,                      //
        "result", glz::escaped<&T::result> //
    );
};

namespace saucer::serializers
{
    glaze::~glaze() = default;

    std::string glaze::script() const
    {
        return {};
    }

    std::string glaze::js_serializer() const
    {
        return "JSON.stringify";
    }

    template <typename T>
    tl::expected<T, glz::error_ctx> parse_as(const std::string &buffer)
    {
        static constexpr auto opts = glz::opts{.error_on_missing_keys = true, .raw_string = false};

        T value{};
        auto error = glz::read<opts>(value, buffer);

        if (error)
        {
            return tl::make_unexpected(error);
        }

        return value;
    }

    std::unique_ptr<message_data> glaze::parse(const std::string &data) const
    {
        if (auto res = parse_as<glaze_function_data>(data); res.has_value())
        {
            return std::make_unique<glaze_function_data>(res.value());
        }

        if (auto res = parse_as<glaze_result_data>(data); res.has_value())
        {
            return std::make_unique<glaze_result_data>(res.value());
        }

        return nullptr;
    }
} // namespace saucer::serializers
