#pragma once

#include "serializer.h"

#include <saucer/serializers/serializer.hpp>

struct saucer_result_data : saucer::result_data
{
    void *user_data;
};

struct saucer_function_data : saucer::function_data
{
    void *user_data;
};

struct saucer_serializer : public saucer::serializer
{
    ~saucer_serializer() override = default;

  public:
    std::string m_script;
    std::string m_js_serializer;

  public:
    saucer_serializer_parser m_parser;

  public:
    [[nodiscard]] std::string script() const override;
    [[nodiscard]] std::string js_serializer() const override;

  public:
    [[nodiscard]] parse_result parse(const std::string &data) const override;
};
