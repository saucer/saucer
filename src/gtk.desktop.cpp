#include "desktop.hpp"

#include "gtk.utils.hpp"

#include <filesystem>

#include <gtk/gtk.h>

namespace saucer
{
    namespace fs = std::filesystem;

    void desktop::open(const std::string &uri)
    {
        if (fs::exists(uri))
        {
            auto file     = utils::g_object_ptr<GFile>{g_file_new_for_path(uri.c_str())};
            auto launcher = utils::g_object_ptr<GtkFileLauncher>{gtk_file_launcher_new(file.get())};

            return gtk_file_launcher_launch(launcher.get(), nullptr, nullptr, nullptr, nullptr);
        }

        auto launcher = utils::g_object_ptr<GtkUriLauncher>{gtk_uri_launcher_new(uri.c_str())};
        gtk_uri_launcher_launch(launcher.get(), nullptr, nullptr, nullptr, nullptr);
    }
} // namespace saucer
