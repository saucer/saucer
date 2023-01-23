#include "../embedding/all.hpp"
#include <saucer/webview.hpp>

int main()
{
    saucer::webview webview;
    webview.set_title("Automated Embedding!");
    webview.embed_files(std::move(embedded::get_all_files()));

    webview.serve("src/index.html");
    webview.set_size(500, 600);
    webview.show();
    webview.run();

    return 0;
}
