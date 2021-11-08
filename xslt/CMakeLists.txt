# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2010-2021 Thomas Fischer <fischer@unix-ag.uni-kl.de>

install(
    FILES
    fancy.xsl
    standard.xsl
    abstractonly.xsl
    arxiv2bibtex.xsl
    pubmed2bibtex.xsl
    pam2bibtex.xsl
    ieeexploreapiv1-to-bibtex.xsl
    oaizbpreview-to-bibtex.xsl
    wikipedia-cite.xsl
    DESTINATION
    ${KDE_INSTALL_DATADIR}/kbibtex
)
