#include <saucer/webview.hpp>
#include "../embedded/all.hpp"

int main()
{
    saucer::webview webview;

    webview.set_size(500, 600);
    webview.set_title("Automated Embedding!");
    webview.embed(saucer::embedded::all());

    webview.serve("src/index.html");
    webview.set_dev_tools(true);
    webview.show();
    webview.run();

    return 0;
}
