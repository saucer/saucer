#include <saucer/webview.hpp>

#include "../embedded/all.hpp"

coco::stray start(saucer::application *app)
{
    auto webview = saucer::webview{{
        .application = app,
    }};

    webview.set_title("Automated Embedding - 還有 Unicode！");
    webview.set_size(500, 600);

    webview.execute(R"js(
        const text = document.createElement('p');
        text.innerText = "來自 C++ 和 Unicode 的問候！";
        
        document.body.append(text);
        )js");

    webview.embed(saucer::embedded::all());
    webview.serve("/src/index.html");

    webview.set_dev_tools(true);
    webview.show();

    co_await app->finish();
}

int main()
{
    return saucer::application::create({.id = "embedding"})->run(start);
}
