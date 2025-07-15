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

### Community

Get in touch: [Discord](https://discord.gg/ndhmQE4225), [Matrix](https://matrix.to/#/#saucer:matrix.org)

### Mirrors

Saucer is also available on: [Codeberg](https://codeberg.org/saucer/saucer)

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

* 👾 Feature-Rich
  > Supports custom schemes, script injection, favicons, and much more!

* 🔀 Seamless C++ / JavaScript interoperability

* 🚀 Coroutine support

* 📦 Supports Embedding Frontend into binary

* 🪶 Lightweight
  > By using the operating systems native web renderer _(or a commonly used one)_, it is possible to create binaries as small as ~250KB

* 🧰 Customizability
  > Modules allow access to platform specific implementations, making it possible to tweak the library to your hearts content
  
  * Official Modules
    * 🖥️ [saucer/desktop](https://github.com/saucer/desktop) 
      > 📂 File-Picker, Mouse-Position retrieval and URI-Launch support

    * 🖨️ [saucer/pdf](https://github.com/saucer/pdf) 
      > 📄 Print current page as PDF

* 🏗️ [Bindings](https://github.com/saucer/bindings)
  > Saucer also exposes a C-Interface, thus making it possible to write bindings for it in your favorite language!  
  
  * Community Bindings
    
    | Language | Repository                         |
    | -------- | ---------------------------------- |
    | Java     | https://github.com/saucer/saucer4j |
    | PHP      | https://github.com/boson-php       |
    | Rust     | https://github.com/skjsjhb/saucers |

    _Please note: All bindings are community maintained!_

* 🦺 Thread-Safe

* 🧨 No Exceptions 
  > Compiles with `-fno-exceptions`!

* 🏃 No RTTI
  > Compiles with `-fno-rtti`!

* ⚖️ FOSS
  > Licensed under MIT!

* 🪟 Built-in support for frame-less windows and transparency

* [... and more!](https://saucer.github.io/)

## ✍️ Code Example

```cpp
#include <saucer/smartview.hpp>

coco::stray start(saucer::application *app)
{
    auto webview = saucer::smartview{{
        .application = app,
    }};

    webview.set_size(900, 700);
    webview.set_title("Hello World!");

    webview.expose("add_random", [&](float number) -> coco::task<float>
    {
        auto random = co_await webview.evaluate<float>("Math.random()");
        co_return number + random;
    });

    webview.set_file("index.html");
    webview.show();

    co_await app->finish();
}

int main()
{
    return saucer::application::create({.id = "example"})->run(start);
}
```

## 🌐 Who's using saucer?

<div align="center">
<br/>

<a href="https://casterlabs.co/" target="_blank">
    <picture>
        <source media="(prefers-color-scheme: dark)" srcset="https://cdn.casterlabs.co/branding/casterlabs/wordmark_white.svg">
        <img width="300" src="https://cdn.casterlabs.co/branding/casterlabs/wordmark_black.svg">
    </picture>
</a>
</div>

<br/>

> [🎉 Become part of this list!](https://github.com/saucer/saucer/issues/new)

## ⭐ Star History


![](https://api.star-history.com/svg?repos=saucer/saucer&type=Date)

## 🍵 Buy me a tea!

Saucer is a passion project and I develop it in my free-time. If you'd like to support me, consider [sponsoring](https://github.com/sponsors/Curve) my tea-addiction 🫂
