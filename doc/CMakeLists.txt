# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2009-2021 Thomas Fischer <fischer@unix-ag.uni-kl.de>
# SPDX-FileContributor: 2020 Pino Toscano <pino@kde.org>

# Create and install documentation (handbook)

kdoctools_create_handbook(
    index.docbook
    INSTALL_DESTINATION ${KDE_INSTALL_DOCBUNDLEDIR}/en
    SUBDIR kbibtex
)

# Create and install documentation (man page)

kdoctools_create_manpage(
    man-kbibtex.1.docbook 1
    INSTALL_DESTINATION ${KDE_INSTALL_MANDIR}
)
