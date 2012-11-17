<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform' xmlns:arxiv="http://arxiv.org/schemas/atom">

<!--
  - This Extensible Stylesheet Language Transformation file translates XML files
  - as provided by IEEE Xplore's XML gateway into BibTeX files.
  - http://ieeexplore.ieee.org/gateway/
  -
  - This file was written by Thomas Fischer <fischer@unix-ag.uni-kl.de>
  - It is released under the GNU Public License version 2 or later.
  -
  - To run test this transformation file, run e.g.
  - wget 'http://ieeexplore.ieee.org/gateway/ipsSearch.jsp?querytext=java&au=Wang&hc=10&rs=11&sortfield=ti&sortorder=asc' -O - | xsltproc  arxiv2bibtex.xsl -
  - Within KBibTeX, some post-processing on the resulting BibTeX file is done.
  -->

<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>
<xsl:strip-space elements="*"/>


<!-- START HERE -->
<xsl:template match="/">
<!-- process each entry -->
<xsl:apply-templates select="root/document" />
</xsl:template>



<xsl:template match="document">

<!-- Determine publication type, use @misc as fallback -->
<xsl:choose>
<xsl:when test="pubtype='Conference Publications'">
<xsl:text>@inproceedings</xsl:text>
</xsl:when>
<xsl:when test="pubtype='Journals &amp; Magazines'">
<xsl:text>@article</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>@misc</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:text>{ieee</xsl:text><xsl:value-of select="arnumber" />

<!-- Title -->
<xsl:if test="title"><xsl:text>,
   title={{</xsl:text><xsl:value-of select="title" /><xsl:text>}}</xsl:text></xsl:if>

<!-- Authors -->
<!-- FIXME replace ';' by 'and' in author list; rename field from "x-author" to "author" -->
<xsl:if test="authors"><xsl:text>,
   x-author={</xsl:text><xsl:value-of select="authors" /><xsl:text>}</xsl:text></xsl:if>

<!-- Conference's Title -->
<xsl:if test="pubtitle and pubtype='Conference Publications'"><xsl:text>,
   booktitle={{</xsl:text><xsl:value-of select="pubtitle" /><xsl:text>}}</xsl:text></xsl:if>

<!-- Journal's Title -->
<xsl:if test="pubtitle and pubtype='Journals &amp; Magazines'"><xsl:text>,
   journal={{</xsl:text><xsl:value-of select="pubtitle" /><xsl:text>}}</xsl:text></xsl:if>

<!-- Publisher -->
<xsl:if test="publisher"><xsl:text>,
   publisher={</xsl:text><xsl:value-of select="publisher" /><xsl:text>}</xsl:text></xsl:if>

<!-- Abstract -->
<xsl:if test="abstract"><xsl:text>,
   abstract={</xsl:text><xsl:value-of select="abstract" /><xsl:text>}</xsl:text></xsl:if>

<!-- Year -->
<xsl:if test="py"><xsl:text>,
   year={</xsl:text><xsl:value-of select="py" /><xsl:text>}</xsl:text></xsl:if>

<!-- Pages -->
<xsl:if test="spage"><xsl:text>,
   pages={</xsl:text><xsl:value-of select="spage" />
<xsl:if test="epage"><xsl:text>--</xsl:text><xsl:value-of select="epage" /></xsl:if>
<xsl:text>}</xsl:text></xsl:if>

<!-- ISSN -->
<xsl:if test="issn"><xsl:text>,
   issn={</xsl:text><xsl:value-of select="issn" /><xsl:text>}</xsl:text></xsl:if>

<!-- Number/Issue -->
<xsl:if test="issue"><xsl:text>,
   number={</xsl:text><xsl:value-of select="issue" /><xsl:text>}</xsl:text></xsl:if>

<!-- Volume -->
<xsl:if test="volume"><xsl:text>,
   volume={</xsl:text><xsl:value-of select="volume" /><xsl:text>}</xsl:text></xsl:if>

<!-- ISBN -->
<xsl:if test="isbn"><xsl:text>,
   isbn={</xsl:text><xsl:value-of select="isbn" /><xsl:text>}</xsl:text></xsl:if>

<!-- DOI -->
<xsl:if test="doi"><xsl:text>,
   doi={</xsl:text><xsl:value-of select="doi" /><xsl:text>}</xsl:text></xsl:if>

<!-- PDF -->
<xsl:if test="pdf"><xsl:text>,
   url={</xsl:text><xsl:value-of select="pdf" /><xsl:text>}</xsl:text></xsl:if>

<xsl:text>
}

</xsl:text>

</xsl:template>

</xsl:stylesheet>
