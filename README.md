# KBibTeX

[![Screenshot of KBibTeX](https://userbase.kde.org/images.userbase/thumb/7/7f/20150602-kbibtex-kf5.png/320px-20150602-kbibtex-kf5.png)](https://userbase.kde.org/images.userbase/7/7f/20150602-kbibtex-kf5.png)

Copyright: 2004–2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>

Author/Maintainer: Thomas Fischer <fischer@unix-ag.uni-kl.de>

This software, unless noted differently for individual files, materials,
or contributions, is licensed under the terms of the GNU General Public
License as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.


## Description

The program KBibTeX is a bibliography editor by KDE. Its main
purpose is to provide a user-friendly interface to BibTeX files.


## Homepage

Visit [https://userbase.kde.org/KBibTeX]() for more information.


## Compiling the Code Yourself

The [development page](https://userbase.kde.org/KBibTeX/Development)
of KBibTeX explains some of the technical details on how to compile
and install KBibTeX itself and how its Git repository is structured.

In case you are impatient and want to have quick results, follow the
[Quick Start to Run KBibTeX from Git](https://userbase.kde.org/KBibTeX/Development#Quick_Start_to_Run_KBibTeX_from_Git) instructions.


## Reporting a Bug

KBibTeX makes use of KDE's own bug reporting system. You'll need
to create an account there to report bugs.

Please visit this page to report your bug or feature request:

[https://bugs.kde.org/enter_bug.cgi?product=KBibTeX]()


## Dependencies

KBibTeX makes heavily use of KDE Frameworks 5 and therefore Qt 5.

In detail, the following dependencies exist:

* Compile time
    - Qt 5.9 or later,
      required components: Core, Widgets, Network, XmlPatterns,
      Concurrent, and NetworkAuth,
      optionally either Qt5WebEngineWidgets or Qt5WebKitWidgets,
      and Test
    - KDE Frameworks 5.51 or later,
      required components: I18n, XmlGui, KIO, IconThemes, ItemViews,
      Parts, CoreAddons, Service, TextEditor,, DocTools, Wallet, Crash,
      and has helper package: extra-cmake-modules (ECM)
    - Poppler (any recent version should suffice) with Qt5 bindings,
    - Optionally, ICU (any recent version should suffice),
      required components: uc and i18n

* Runtime
    - BibUtils to import/export various bibliography formats
    - A TeX distribution including pdflatex, bibtex, ...

In case this list is incomplete, wrong, or out-dated, please file
a bug for KBibTeX at [https://bugs.kde.org/enter_bug.cgi?product=KBibTeX]().
