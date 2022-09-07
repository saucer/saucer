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
    using serialize_callback_t = bool (*)(saucer::smartview *, saucer::function_data *, char *, std::size_t, std::size_t *, int *);

  public:
    ~serializer() override;

  public:
    std::string initialization_script() const override;
    std::string java_script_serializer() const override;
    std::shared_ptr<saucer::message_data> parse(const std::string &data) const override;

  public:
    static std::function<tl::expected<std::string, error>(const std::shared_ptr<saucer::function_data> &)> serialize_function(saucer::smartview *smartview);

  public:
    static std::size_t buffer_size;
    static std::string init_script;
    static std::string js_serializer;
    static parse_callback_t parse_callback;
    static serialize_callback_t serialize_callback;
};