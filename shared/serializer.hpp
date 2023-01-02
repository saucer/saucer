#pragma once
#include <memory>
#include <cstdint>
#include <saucer/smartview.hpp>

struct ffi_function_data : public saucer::function_data
{
    void *data;
    ~ffi_function_data() override;
};

struct ffi_result_data : public saucer::result_data
{
    void *data;
    ~ffi_result_data() override;
};

struct serializer : public saucer::serializer
{
    using parse_callback_t = saucer::message_data *(*)(const char *);
    using serialize_callback_t = bool (*)(saucer::smartview *, saucer::function_data *, char *, std::size_t,
                                          std::size_t *, int *);

  public:
    ~serializer() override;

  public:
    [[nodiscard]] std::string init_script() const override;
    [[nodiscard]] std::string js_serializer() const override;

  public:
    [[nodiscard]] std::unique_ptr<saucer::message_data> parse(const std::string &data) const override;

  public:
    static serializer::resolve_callback serialize_function(saucer::smartview *smartview);

  public:
    static std::string m_script;
    static std::string m_serializer;

  public:
    static std::size_t buffer_size;
    static parse_callback_t parse_callback;
    static serialize_callback_t serialize_callback;
};