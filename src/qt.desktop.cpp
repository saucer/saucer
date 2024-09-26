#include "desktop.hpp"

#include <QUrl>
#include <QDesktopServices>

namespace saucer
{
    void desktop::open(const std::string &uri)
    {
        QDesktopServices::openUrl(QString::fromStdString(uri));
    }
} // namespace saucer
