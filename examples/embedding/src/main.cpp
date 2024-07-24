#include <iostream>
#include <saucer/webview.hpp>
#include "../embedded/all.hpp"

int main()
{
    saucer::webview::register_scheme("test");
    saucer::webview webview;

    webview.handle_scheme("test",
                          [](const saucer::request &req)
                          {
                              std::cout << "User requested url: " << req.url() << std::endl;

                              std::string response = R"html(
                               <!DOCTYPE html>
                               <html>
                                <body>
                                    <h1>My First Heading</h1>
                                    <p>My first paragraph.</p>
                                </body>
                               </html>
                              )html";

                              return saucer::response{
                                  .mime = "text/html",
                                  .data = saucer::stash<const std::uint8_t>::from({response.begin(), response.end()}),
                              };
                          });

    webview.set_title("Automated Embedding - 還有 Unicode！");
    webview.embed(saucer::embedded::all());
    webview.set_size(500, 600);

    webview.execute(R"js(
        const text = document.createElement('p');
        text.innerText = "來自 C++ 和 Unicode 的問候！";

        document.body.append(text);
    )js");

    webview.serve("index.html", "test");
    // webview.set_dev_tools(true);
    webview.show();
    webview.run();

    return 0;
}
