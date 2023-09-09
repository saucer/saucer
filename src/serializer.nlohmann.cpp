#include "serializers/nlohmann/nlohmann.hpp"

namespace saucer::serializers
{
    nlohmann::~nlohmann() = default;

    std::string nlohmann::script() const
    {
        return {};
    }

    std::string nlohmann::js_serializer() const
    {
        return "JSON.stringify";
    }

    std::unique_ptr<message_data> nlohmann::parse(const std::string &data) const
    {
        auto parsed = ::nlohmann::json::parse(data, nullptr, false);

        if (parsed.is_discarded())
        {
            return nullptr;
        }

        if (!parsed["id"].is_number_integer())
        {
            return nullptr;
        }

        if (parsed["params"].is_array() && parsed["name"].is_string())
        {
            auto rtn = std::make_unique<nlohmann_function_data>();

            rtn->id = parsed["id"];
            rtn->name = parsed["name"];
            rtn->data = parsed["params"];

            return rtn;
        }

        if (!parsed["result"].is_discarded())
        {
            auto rtn = std::make_unique<nlohmann_result_data>();

            rtn->id = parsed["id"];
            rtn->data = parsed["result"];

            return rtn;
        }

        return nullptr;
    }
} // namespace saucer::serializers