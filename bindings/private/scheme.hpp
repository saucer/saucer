#pragma once

#include "handle.hpp"
#include <saucer/scheme.hpp>

struct saucer_response : bindings::handle<saucer_response, saucer::scheme_handler::result_type>
{
};

struct saucer_request : bindings::handle<saucer_request, const saucer::request *>
{
};
