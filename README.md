<hr>

<div align="center"> 
    <img src="https://raw.githubusercontent.com/saucer/saucer.github.io/master/static/img/logo.png" height="312" />
</div>

<p align="center"> 
    Build cross-platform desktop apps with C++ & Web Technologies 
</p>

---

<div align="center"> 

<img src="https://raw.githubusercontent.com/saucer/saucer.github.io/rewrite/static/assets/preview.png" width="600" />

### Documentation

The documentation can be found [here](https://saucer.github.io/).

### Examples

Examples can be found [here](./examples).

### Getting started

Learn how to create your first _saucer_ app [here](https://saucer.github.io/docs/getting-started).

</div> 

---

## 👽 Introduction

Saucer is a modern C++ webview library that allows you to build cross-platform desktop applications with ease.  
It supports all three major desktop operating systems (Windows, Linux, MacOS) and has several backend implementations.

## 🛸 Features

* 💻 Cross Platform
  
  <table>
    <tr>
      <th></th>
      <th>Windows</th>
      <th>Linux</th>
      <th>MacOS</th>
    </tr>
    <tr>
      <td rowspan="2">Backend</td>
      <td>Win32 & WebView2</td>
      <td>GTK4 & WebKitGtk</td>
      <td>Cocoa & WKWebView</td>
    </tr>
    <tr align="center">
      <td colspan="3">Qt5 / Qt6 & QWebEngine</td>
    </tr>
  </table>

* 🔀 Seamless C++ / JavaScript interoperability

* 📦 Supports Embedding Frontend into binary

* 🪶 Lightweight
  > By using the operating systems native web renderer _(or a commonly used one)_, it is possible to create binaries as small as ~250KB

* 🧰 Customizability
  > Modules allow access to platform specific implementations, making it possible to tweak the library to your hearts content

* 🏗️ [Bindings](https://github.com/saucer/bindings)
  > Saucer also exposes a C-Interface, thus making it possible to write bindings for it in your favorite language!

* 🦺 Thread-Safe

* ⚖️ FOSS
  > Licensed under MIT!

* 🪟 Built-in support for frame-less windows and transparency

* [... and more!](https://saucer.github.io/)

## ✍️ Code Example

```cpp
#include <saucer/smartview.hpp>

int main()
{
    auto app = saucer::application::acquire({
        .id = "example",
    });

    saucer::smartview webview{{
        .application = app,
    }};

    webview.set_size(900, 700);
    webview.set_title("Hello World!");

    webview.expose(
        "add_random",
        [&](float number)
        {
            auto random = webview.evaluate<float>("Math.random()").get();
            return number + random;
        },
        saucer::launch::async);

    webview.set_file("index.html");
    
    webview.show();
    app->run();

    return 0;
}
```
