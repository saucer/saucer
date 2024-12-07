#include <boost/ut.hpp>
#include <saucer/webview.hpp>

int main()
{
    saucer::webview::register_scheme("test");

    auto app = saucer::application::init({
        .id = "app.saucer.tests",
    });

    return boost::ut::cfg<>.run();
}
