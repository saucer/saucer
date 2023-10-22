#include <saucer/webview.hpp>
#include "../embedded/all.hpp"

int main()
{
    saucer::webview webview;

    webview.set_title("Automated Embedding - 還有 Unicode！");
    webview.embed(saucer::embedded::all());
    webview.set_size(500, 600);

    webview.run_java_script(R"js(
        const text = document.createElement('p');
        text.innerText = "來自 C++ 和 Unicode 的問候！";

        document.body.append(text);
    )js");

    webview.serve("src/index.html");
    webview.set_dev_tools(true);
    webview.show();
    webview.run();

    return 0;
}
