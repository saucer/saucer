#include "desktop.hpp"

#include <format>
#include <filesystem>

#include <gtk/gtk.h>

namespace saucer
{
    namespace fs = std::filesystem;

#if GTK_CHECK_VERSION(4, 10, 0)
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
#else
    void desktop::open(const std::string &uri)
    {
        auto target = uri;

        if (fs::exists(uri))
        {
            target = std::format("file://{}", uri);
        }

        gtk_show_uri(nullptr, target.c_str(), GDK_CURRENT_TIME);
    }
#endif
} // namespace saucer
