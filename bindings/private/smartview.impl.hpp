#pragma once

#include "serializer.h"

#include <saucer/smartview.hpp>

struct saucer_handle : public saucer::smartview_core
{
    saucer::serializer::function m_function;
    saucer::serializer::resolver m_resolver;

  public:
    saucer_handle(saucer_serializer *, const saucer::options & = {});

  public:
    using saucer::smartview_core::add_evaluation;
    using saucer::smartview_core::add_function;
};
