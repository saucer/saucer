<hr>

<div align="center">
    <img src="https://raw.githubusercontent.com/saucer/saucer.github.io/master/static/img/logo.png" height=312/>
</div>

<p align="center">
    <b>Welcome to our contribution guidelines!</b>
    <br/>
    <br/>
    We're happy you want to contribute!
    Please try to adhere to the following rules to help us maintain a clean codebase and to make the merge-process as simple as possible!
</p>

---

# Talk to us!

Before starting to work on any patches please talk to us before hand.\
This helps us prevent situations where you may want to implement a feature that is already in the works.

We're always grateful for your contributions :) We just want to make sure your hard work doesn't go to waste!

<br/>
<div align="center">
  <a href="https://discord.gg/ndhmQE4225">
    <img src="https://invidget.switchblade.xyz/ndhmQE4225" alt="Discord Invite"/>
  </a>
</div>
<br/>

# Semantic Commits

Please always use [semantic commit messages](https://www.conventionalcommits.org/en/v1.0.0/), so that we can keep track of the changes more easily!

# Code Style

We follow a specific coding-style throughout the project:

- Includes
  - Includes should be sorted by length where applicable
  - Project/Internal Headers always come first
  - System and Project Headers should be separated by a line break
  - If applicable, separate includes into reasonable groups

- Naming Convention
  - We use a "standard library-ish" naming convention
    > _As there is no such definitive convention, it is sometimes up to debate how to name things, but if you look through the source code you should get the gist of how to name things, it's mostly just <kbd>snake_case</kbd>, except for Template-Parameters, which should be in <kbd>CamelCase</kbd>._

- Nesting
  - Function depth should be kept to a minimum
  - In cases where nesting increases the overview of the functions flow or is absolutely necessary, it may be used unless it is explicitly requested to not do so

- File/Folder Structure
  - Implementation specific headers should be placed in the <kbd>private</kbd> directory
  - For each implementation header there should be a corresponding source file
  - For all public headers a ".inl" file should be preferred over inline definitions
  - Files should include their name and the operating-system they're designed for _(only if applicable!)_ separated by a dot (.)

- Clang-Tidy / Clang-Format
  - Before you commit your changes make sure that there are no clang-tidy warnings and format your code with our `.clang-format` config

- Annotations
  - All functions exposed to the user should be annotated accordingly _(only if applicable)_ _[for an example see [webview.hpp](include/saucer/webview.hpp)]_

- Artifial Intelligence
  - All contributions must not use AI. Saucer is 100% human written and intends to stay that way.\
    Thus, when submitting a Pull Request, please also acknowledge that you have not used AI.
