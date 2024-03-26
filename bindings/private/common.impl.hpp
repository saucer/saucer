#pragma once

#include "options.h"
#include "serializer.h"

#include <saucer/smartview.hpp>

struct saucer_handle : public saucer::smartview_core
{
    saucer_handle(saucer_serializer *, saucer_options *);
};
