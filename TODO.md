*INFO* This TODO list will most likely outdate _very_ quickly.

# General/High-Level Goals

* Migrate and consistently make use of more modern C++ features.
* First prepare (2020) and later migrate to Qt6 and KF6 (2021+).

## Code Maintenance

* Whenever making changes, the same commit should contain a ChangeLog
  update, either by updating the ChangeLog file immediately or/and by
  using a specially-crafted line in the Git commit message that allows
  for automatic processing when preparing for a release.
* Whenever making changes, write automated tests that confirm that a
  feature works as expected with known input/output or if a bug is
  actually fixed.s

## Code Simplifications

_There are probably numerous places with issues like those listed below._

* Fix bug 356142: The complex file opening flow: need refactored?

## Continuous Integration and Non-Linux Operating Systems

* Make KBibTeX compile and run on operating systems which are not Linux
  such as *BSD, Windows, or MacOS.
* Automatically build installers/packages for popular distributions or
  operating systems by utilizing KDE's and third party CI infrastructure.

# User Interface

* Better support BibLaTeX and its numerous fields. This includes
  re-designing and re-thinking the user interface and checking user input
  for consistency (e.g. not all fields may be used at all time or there
  may be dependencies/references between entries).
* Maintain/improve QML-based interfaces for SailfishOS and Kirigami.
