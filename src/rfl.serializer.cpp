#include "serializers/rflpp/rflpp.hpp"

#include <optional>

namespace rfl
{
    using namespace saucer::serializers::rflpp;

    using function_data = serializer::function_data;
    using result_data   = serializer::result_data;

    template <>
    struct Reflector<function_data>
    {
        struct ReflType
        {
            rfl::Rename<"saucer:call", bool> tag;
            std::size_t id;
            std::string name;
        };

        static function_data to(const ReflType &v) noexcept
        {
            function_data rtn;

            rtn.id   = v.id;
            rtn.name = v.name;

            return rtn;
        }
    };

    template <>
    struct Reflector<result_data>
    {
        struct ReflType
        {
            rfl::Rename<"saucer:resolve", bool> tag;
            std::size_t id;
        };

        static result_data to(const ReflType &v) noexcept
        {
            result_data rtn;

            rtn.id = v.id;

            return rtn;
        }
    };
} // namespace rfl

namespace saucer::serializers::rflpp
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
    std::optional<T> parse_as(std::string_view buffer)
    {
        auto result = rfl::json::read<T>(buffer);

        if (!result)
        {
            return std::nullopt;
        }

        return result.value();
    }

    serializer::parse_result serializer::parse(std::string_view data) const
    {
        if (auto res = parse_as<function_data>(data); res.has_value())
        {
            auto rtn = std::make_unique<function_data>(res.value());
            rtn->raw = data;
            return rtn;
        }

        if (auto res = parse_as<result_data>(data); res.has_value())
        {
            auto rtn = std::make_unique<result_data>(res.value());
            rtn->raw = data;
            return rtn;
        }

        return std::monostate{};
    }
} // namespace saucer::serializers::rflpp
