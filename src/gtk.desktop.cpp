#include "desktop.hpp"

#include <filesystem>

#include <gtk/gtk.h>

namespace saucer
{
    namespace fs = std::filesystem;

    void desktop::open(const std::string &uri)
    {
        if (fs::exists(uri))
        {
            auto *file     = g_file_new_for_path(uri.c_str());
            auto *launcher = gtk_file_launcher_new(file);

            gtk_file_launcher_launch(launcher, nullptr, nullptr, nullptr, nullptr);

            g_object_unref(file);
            g_object_unref(launcher);

            return;
        }

        auto *launcher = gtk_uri_launcher_new(uri.c_str());
        gtk_uri_launcher_launch(launcher, nullptr, nullptr, nullptr, nullptr);

        g_object_unref(launcher);
    }
} // namespace saucer
