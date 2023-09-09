#include "serializers/glaze/glaze.hpp"

template <>
struct glz::meta<saucer::serializers::glaze_function_data>
{
    using T = saucer::serializers::glaze_function_data;
    static constexpr auto value = object( //
        "id", &T::id,                     //
        "name", &T::name,                 //
        "params", &T::params              //
    );
};

template <>
struct glz::meta<saucer::serializers::glaze_result_data>
{
    using T = saucer::serializers::glaze_result_data;
    static constexpr auto value = object( //
        "id", &T::id,                     //
        "result", &T::result              //
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
    auto parse_as(const std::string &buffer)
    {
        static constexpr auto opts = glz::opts{.error_on_missing_keys = true};

        T value;
        auto error = glz::read<opts>(value, buffer);

        return std::make_pair(error, value);
    }

    std::unique_ptr<message_data> glaze::parse(const std::string &data) const
    {
        if (auto [error, value] = parse_as<glaze_function_data>(data); !error)
        {
            return std::make_unique<glaze_function_data>(value);
        }

        if (auto [error, value] = parse_as<glaze_result_data>(data); !error)
        {
            return std::make_unique<glaze_result_data>(value);
        }

        return nullptr;
    }
} // namespace saucer::serializers