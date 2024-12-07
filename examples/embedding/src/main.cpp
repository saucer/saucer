#include <saucer/webview.hpp>
#include "../embedded/all.hpp"

int main()
{
    auto app = saucer::application::init({
        .id = "embedding",
    });

    saucer::webview webview{{
        .application = app,
    }};

    webview.set_title("Automated Embedding - 還有 Unicode！");
    webview.embed(saucer::embedded::all());
    webview.set_size(500, 600);

    webview.execute(R"js(
        const text = document.createElement('p');
        text.innerText = "來自 C++ 和 Unicode 的問候！";

        document.body.append(text);
    )js");

    webview.serve("src/index.html");
    webview.set_dev_tools(true);

    webview.show();
    app->run();

    return 0;
}
