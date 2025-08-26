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

Saucer is available on: [GitHub](https://github.com/saucer/saucer), [Codeberg](https://codeberg.org/saucer/saucer)

</div> 

---

## 👽 Introduction

Saucer is a modern C++ webview library that allows you to build cross-platform desktop applications with ease.  
It supports all three major desktop operating systems (Windows, Linux, MacOS) and has several backend implementations.

## 🛸 Features

* 💻 Cross Platform
  
  <table>
    <tr>
      <th>Platform</th>
      <th colspan="2">Backends</th>
    </tr>
    <tr>
      <td><b>Windows</b></td>
      <td>Win32 & WebView2</td>
      <td rowspan="3">Qt5 / Qt6 + QWebEngine</td>
    </tr>
    <tr>
      <td><b>Linux</b></td>
      <td>GTK4 & WebKitGtk</td>
    </tr>
    <tr>
      <td><b>MacOS</b></td>
      <td>Cocoa & WKWebView</td>
    </tr>
  </table>

* 👾 Feature-Rich
  > Supports custom schemes, script injection, favicons, and much more!

* ㊗️ Unicode Support
  > Uses UTF-8 encoded strings

* 🔀 Seamless C++ / JavaScript interoperability

* 🚀 Coroutine support

* 📦 Supports Embedding Frontend into binary

* 🪶 Lightweight
  > By using the operating systems native web renderer[^1], it is possible to create binaries as small as ~250KB

* 🧰 Customizability
  > Supports a convenient API to access platform specific implementations, making it possible to tweak the library to your hearts content!
  
  * Official Modules
    * 🖥️ [saucer/desktop](https://github.com/saucer/desktop)
      > 📂 File-Picker, Mouse-Position retrieval and URI-Launch support  
      >  ⮱ [Example](./examples/desktop)

    * 🖨️ [saucer/pdf](https://github.com/saucer/pdf) 
      > 📄 Export current page as PDF  
      >  ⮱ [Example](./examples/pdf)

    * ➰ [saucer/loop](https://github.com/saucer/loop)
      > 🛞 "Legacy"[^2] loop implementation  
      >  ⮱ [Example](./examples/loop)

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

* 🦥 [... and more!](https://saucer.github.io/)

[^1]: ... or a commonly used one
[^2]: For lack of a better word

## ✍️ Code Example

```cpp
#include <print>
#include <saucer/smartview.hpp>

coco::stray start(saucer::application *app)
{
    auto window  = saucer::window::create(app).value();
    auto webview = saucer::smartview<>::create({.window = window});

    window->set_title("Hello World!");
    window->set_size({.w = 800, .h = 600});

    webview->expose("add_demo", [&](double a, double b) -> coco::task<double>
    { 
        co_return a + b + co_await webview->evaluate<double>("Math.random()"); 
    });

    auto index = saucer::uri::from("index.html");

    if (!index.has_value())
    {
        co_return std::println("{}", index.error());
    }

    webview->set_url(index.value());
    window->show();

    co_await app->finish();
}

int main()
{
    return saucer::application::create({.id = "example"})->run(start);
}
```

> 🔍 See more [examples](./examples)!

## 🌐 Who's using saucer?

<div align="center">

| <img src="https://casterlabs.co/images/product/caffeinated/multi-chat.png" width="1000" /> | <img src="https://raw.githubusercontent.com/SamsidParty/TopNotify/main/Docs/Screenshot3.png" width="1250" /> |
| :----------------------------------------------------------------------------------------: | :----------------------------------------------------------------------------------------------------------: |
|                     [Casterlabs - Caffeinated](https://casterlabs.co/)                     |                            [TopNotify](https://github.com/SamsidParty/TopNotify)                             |
|                _(Built on [saucer4j](https://github.com/saucer/saucer4j))_                 |                              _(Built on their C# Saucer Bindings "IgniteView")_                              |

</div>

<br/>

> [🎉 Become part of this list!](https://github.com/saucer/saucer/issues/new)

## ⭐ Star History


![](https://api.star-history.com/svg?repos=saucer/saucer&type=Date)

## 🍵 Buy me a tea!

Saucer is a passion project and I develop it in my free-time. If you'd like to support me, consider [sponsoring](https://github.com/sponsors/Curve) my tea-addiction 🫂
