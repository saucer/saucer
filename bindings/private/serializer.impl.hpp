#pragma once

#include "serializer.h"

#include <saucer/serializers/serializer.hpp>

struct saucer_message_data : saucer::message_data
{
};

struct saucer_result_data : saucer::result_data
{
    void *user_data;
};

struct saucer_function_data : saucer::function_data
{
    void *user_data;
};

struct saucer_parse_result
{
    virtual ~saucer_parse_result() = default;
};

struct saucer_parse_result_expected : public saucer_parse_result
{
    std::string code;
};

struct saucer_parse_result_unexpected : public saucer_parse_result
{
    std::unique_ptr<saucer::error> error;
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
    saucer_serializer_resolver m_resolver;
    saucer_serializer_function m_function;

  public:
    [[nodiscard]] std::string script() const override;
    [[nodiscard]] std::string js_serializer() const override;

  public:
    [[nodiscard]] parse_result parse(const std::string &data) const override;
};
