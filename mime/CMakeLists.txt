# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2011-2021 Thomas Fischer <fischer@unix-ag.uni-kl.de>

find_package(
    SharedMimeInfo
)

########### install files ###############
install(
    FILES
    bibliography.xml
    DESTINATION
    ${KDE_INSTALL_MIMEDIR}
)
update_xdg_mimetypes(
    ${KDE_INSTALL_MIMEDIR}
)
