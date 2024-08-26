#include "cfg.hpp"

#include <saucer/webview.hpp>

int main()
{
    saucer::webview::register_scheme("test");
    return boost::ut::cfg<boost::ut::override>.run();
}
