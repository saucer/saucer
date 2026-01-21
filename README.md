<hr>

<div align="center">
    <img src="https://github.com/saucer/saucer.github.io/blob/v7/src/assets/logo-alt.svg?raw=true" height="312" />
</div>

<p align="center">
    Build small, fast and modern cross-platform desktop apps with C++ & Web Technologies
</p>

---

<div align="center">
<img src="https://github.com/saucer/saucer.github.io/blob/v7/src/assets/preview.png?raw=true" width="600" />

### Documentation

The documentation can be found [here](https://saucer.github.io/).

### Examples

Examples can be found [here](./examples).

### Getting started

Learn how to create your first _saucer_ app [here](https://saucer.github.io/getting-started/).

### Community

Get in touch: [Discord](https://discord.gg/VCF4hNVkn5), [Matrix](https://matrix.to/#/#saucer:matrix.org)

### Mirrors

Saucer is available on: [GitHub](https://github.com/saucer/saucer), [Codeberg](https://codeberg.org/saucer/saucer)

</div>

---

## 👽 Introduction

Saucer is a modern C++ webview library that allows you to build cross-platform desktop applications with ease.\
It supports all three major desktop operating systems (Windows, Linux, MacOS) and has several backend implementations.

## 🛸 Features

- ⚖️ FOSS
  > Licensed under MIT[^4]!

- 🪶 Lightweight
  > By using the operating systems native web renderer[^1], it is possible to create binaries as small as ~250KB

- 🔀 Seamless C++ / JavaScript interoperability
  > Convenient API (powered by reflection) to evaluate JavaScript Code or expose C++ functions to JavaScript!\
  > \> [Example](./examples/expose/main.cpp)

- 👾 Feature-Rich
  > Supports custom schemes, script injection, favicons, and much more!

- ㊗️ Unicode Support

- 🚀 Coroutine Support
  > See [Code Example](#%EF%B8%8F-code-example) below!

- 📦 Supports Embedding Frontend into binary
  > Ship a contained binary with ease!\
  > \> [Documentation](https://saucer.github.io/webview/embedding/)

- 🦺 Thread-Safe

- 🧨 No Exceptions[^3]
  > Compiles with `-fno-exceptions`!

- 🏃 No RTTI
  > Compiles with `-fno-rtti`!

- 🪟 Built-in support for frame-less windows and transparency
  > Supports `data-webview` attributes to allow effortless custom title-bars.\
  > \> [Documentation](https://saucer.github.io/window/decorations/)

- 🧰 Customizable
  > Exposes platform specific implementations, making it possible to tweak the library to your hearts content!\
  > \> [Documentation](https://saucer.github.io/misc/native/)

  Saucer also provides official extensions in the form of modules:

  | Module                                              | Description                                                                                          |
  | --------------------------------------------------- | ---------------------------------------------------------------------------------------------------- |
  | [saucer/desktop](https://github.com/saucer/desktop) | 📂 File-Picker, Mouse-Position retrieval and URI-Launch support <br> > [Example](./examples/desktop) |
  | [saucer/pdf](https://github.com/saucer/pdf)         | 📄 Export current page as PDF <br> > [Example](./examples/pdf)                                       |
  | [saucer/loop](https://github.com/saucer/loop)       | 🛞 "Legacy"[^2] loop implementation <br> > [Example](./examples/loop)                                |

- 💻 Supports various backends

  <table>
    <tr>
      <th>Platform</th>
      <th colspan="2">Backends</th>
    </tr>
    <tr>
      <td><b>Windows</b></td>
      <td>Win32 & WebView2</td>
      <td rowspan="3">Qt + QWebEngine</td>
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

- 🏗️ [Bindings](https://github.com/saucer/bindings)
  > Saucer also exposes a C-Interface, thus making it possible to write bindings for it in your favorite language!

  | Language | Repository                           |
  | -------- | ------------------------------------ |
  | Java     | <https://github.com/saucer/saucer4j> |
  | PHP      | <https://github.com/boson-php>       |
  | Rust     | <https://github.com/skjsjhb/saucers> |

  _Please note: All bindings are community maintained!_

- 🦥 [... and more!](https://saucer.github.io/)

[^1]: ... or a commonly used one

[^2]: For lack of a better word

[^3]: Saucer catches exceptions thrown in exposed functions - **if not compiled with** `-fno-exceptions` - by default. This can be explicitly disabled via `set(saucer_exceptions OFF)`

[^4]: Cannot be licensed under MIT when using the Qt Backend, see [LICENSE](https://github.com/saucer/saucer/blob/master/LICENSE) 

## 📦 Installation

<table>
<tr>
<th>
<a href="https://github.com/cpm-cmake/CPM.cmake" target="_blank">CPM</a>
</th>
<th>
FetchContent
</th>
</tr>
<tr>
<td>

```cmake
CPMFindPackage(
  NAME           saucer
  VERSION        8.0.4
  GIT_REPOSITORY "https://github.com/saucer/saucer"
)
```

</td>
<td>

```cmake
FetchContent_Declare(
  saucer 
  GIT_TAG v8.0.4
  GIT_REPOSITORY "https://github.com/saucer/saucer" 
)

FetchContent_MakeAvailable(saucer)
```

</td>
</tr>
<tr>
<td colspan="2">

**Note**: Replace `<target>` with your respective target:

</td>
</tr>
<tr>
<td colspan="2">

```cmake
target_link_libraries(<target> saucer::saucer)
```

</td>
</tr>
</table>

See the [documentation](https://saucer.github.io/getting-started/) for more information!

## ✍️ Code Example

```cpp
#include <print>
#include <saucer/smartview.hpp>

coco::stray start(saucer::application *app)
{
    auto window  = saucer::window::create(app).value();
    auto webview = saucer::smartview::create({.window = window});

    window->set_title("Hello World!");
    window->set_size({.w = 800, .h = 600});

    webview->expose("add_demo", [&](double a, double b) -> coco::task<double>
    {
        co_return a + b + *co_await webview->evaluate<double>("Math.random()");
    });

    auto index = saucer::url::from("index.html");

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

## 🍵 Buy me a tea

Saucer is a passion project and I develop it in my free-time. If you'd like to support me, consider [sponsoring](https://github.com/sponsors/Curve) my tea-addiction 🫂

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

---

> Saucer is 100% human written. No AI-Tools were used in the development process.

<a href="https://notbyai.fyi/" target="_blank">
  <img align="right" src="https://raw.githubusercontent.com/saucer/saucer.github.io/refs/heads/v7/src/assets/badge.svg" />
</a>
