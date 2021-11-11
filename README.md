<p align="center"> 
<b>⚠️ This readme is under construction and subject to change ⚠️</b>
</p> 


<hr>

<div align="center"> 
    <img src="assets/logo.png" height=312/>
</div>

<p align="center"> 
    Build cross-platform desktop apps with C++ & Web Technologies 
</p>

---

# Introduction
_Saucer_ is a library that aims to provide an easy way to develop desktop applications with Web-Technologies. 
<br/> <br/>
In contrast to other C/++ libraries _Saucer_ provides an easy way to embed all of your required JavaScript & HTML files into a single binary as well as easy interoperability of your C++ and JavaScript code.

---

# Roadmap
- [ ] Easy communication between C++ & JavaScript
- [ ] An easy way to embed all required files
- [ ] User defined communication protocol
- [ ] Custom Drag & Drop handler

---

# Compatibility

| Platform | Supported          | WebView     | Backend | Remarks                                         |
| -------- | ------------------ | ----------- | ------- | ----------------------------------------------- |
| Linux    | :heavy_check_mark: | QtWebEngine | Qt5     | Should work on every platform that Qt supports. |
| Windows  | :x: _(soon)_       | WebView2    | WinAPI  |                                                 |
| MacOS    | :grey_question:    | QtWebEngine | Qt5     | May not work on arm¹                            |

> ¹ Saucer has not yet been tested on macOS. It's solely an assumption that saucer will work with the Qt-Backend on macOS.