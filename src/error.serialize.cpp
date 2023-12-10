#include "serializers/errors/serialize.hpp"

namespace saucer::errors
{
    serialize::~serialize() = default;

    std::string serialize::what()
    {
        return "Unknown serialization error";
    }
} // namespace saucer::errors
