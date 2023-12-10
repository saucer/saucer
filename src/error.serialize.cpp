#include "serializers/errors/unknown.hpp"

namespace saucer::errors
{
    serialize::~serialize() = default;

    std::string serialize::what()
    {
        return "Unknown serialization error";
    }
} // namespace saucer::errors
