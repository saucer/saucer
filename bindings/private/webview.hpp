#pragma once

#include "webview.h"
#include <saucer/webview.hpp>

struct saucer_handle : saucer::webview
{
    saucer_on_message m_on_message;

  public:
    using saucer::webview::webview;

  public:
    bool on_message(const std::string &message) override
    {
        if (saucer::webview::on_message(message))
        {
            return true;
        }

        if (!m_on_message)
        {
            return false;
        }

        return m_on_message(message.c_str());
    }
};
