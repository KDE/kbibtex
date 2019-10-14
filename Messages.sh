#!/bin/sh

# Suggested by Yuri Chornoivan in Git Review Board request 114119
# https://git.reviewboard.kde.org/r/114119/
# to extract strings in configuration files to be translated as well.
# In C++ code used like this:
#   QString translated = i18n(configGroup.readEntry("uiCaption", QString()).toUtf8().constData());
find config -name \*.kbstyle -exec sed -ne '/Label=\|uiLabel\|uiCaption/p' {} \; | sed 's/.*=\([A-Za-z\(\)\/].*\)/i18n("\1");/' >>rc.cpp

# Extract strings for infoMessages as well
find config -name \*.kbstyle -exec sed -ne '/infoMessage/p' {} \; | sed 's/\"/\\"/g' | sed 's/[^=]*=\([A-Za-z\(\)\/].*\)/i18n("\1");/' >>rc.cpp

# Taking instructions from
# http://techbase.kde.org/Development/Tutorials/Localization/i18n_Build_Systems

# invoke the extractrc script on all .ui, .rc, and .kcfg files in the sources
# the results are stored in a pseudo .cpp file to be picked up by xgettext.
$EXTRACTRC `find src -name \*.rc -o -name \*.ui -o -name \*.kcfg | sort -u` >>rc.cpp

# call xgettext on all source files. If your sources have other filename
# extensions besides .cpp and .h, just add them in the find call.
$XGETTEXT `find src -name \*.h -o -name \*.cpp | grep -v '/test/'` rc.cpp -o $podir/kbibtex.pot
