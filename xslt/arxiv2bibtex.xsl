<xsl:stylesheet version='2.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform' xmlns:arxiv="http://arxiv.org/schemas/atom">

<!--
  - This Extensible Stylesheet Language Transformation file translates XML files
  - as provided by arXiv into BibTeX files.
  -
  - This file was written by Thomas Fischer <fischer@unix-ag.uni-kl.de>
  - It is released under the GNU Public License version 2 or later.
  -
  - To run test this transformation file, run e.g.
  - wget 'http://export.arxiv.org/api/query?search_query=all:gandalf+lechner' -O - | sed -e 's/xmlns="http:\/\/www.w3.org\/2005\/Atom"//' | xsltproc  arxiv2bibtex.xsl -
  - Within KBibTeX, some post-processing on the resulting BibTeX file is done.
  -->

<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>
<xsl:strip-space elements="*"/>


<!-- START HERE -->
<xsl:template match="/">
<!-- process each entry -->
<xsl:apply-templates select="feed/entry" />
</xsl:template>



<xsl:template match="entry">

<!-- Test if entry is a journal article (@article), otherwise @misc -->
<xsl:choose>
<xsl:when test="arxiv:journal_ref">
<xsl:text>@article</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>@misc</xsl:text>
</xsl:otherwise>
</xsl:choose>
<xsl:text>{</xsl:text><xsl:value-of select='substring(id,22,100)' />

<!-- process authors by merging all names with "and" -->
<xsl:text>,
    author = {</xsl:text>
<xsl:for-each select="author/name" >
<xsl:if test="position() > 1"><xsl:text> and </xsl:text></xsl:if>
<xsl:value-of select="."/>
</xsl:for-each>
<xsl:text>}</xsl:text>

<xsl:apply-templates select="title" />
<xsl:apply-templates select="updated" />
<xsl:apply-templates select="summary" />
<xsl:if test="link[@type='application/pdf' or @type='text/html']">
<xsl:text>,
    url = {</xsl:text><xsl:value-of select="link[@type='application/pdf' or @type='text/html']/@href" separator=";" /><xsl:text>}</xsl:text>
</xsl:if>
<xsl:apply-templates select="arxiv:doi" />
<xsl:apply-templates select="arxiv:journal_ref" />
<xsl:text>,
    archivePrefix = {arXiv},
    eprint = {</xsl:text>
<xsl:value-of select='substring(id,22,100)' />
<xsl:text>},
    primaryClass = {</xsl:text>
<xsl:value-of select="arxiv:primary_category/@term" />
<xsl:text>},
    comment = {published = </xsl:text>
<xsl:value-of select="published" />
<xsl:if test="updated">
<xsl:text>, updated = </xsl:text>
<xsl:value-of select="updated" />
</xsl:if>
<xsl:if test="arxiv:comment">
<xsl:text>, </xsl:text>
<xsl:value-of select="arxiv:comment" />
</xsl:if>
<xsl:text>}
}

</xsl:text>
</xsl:template>




<xsl:template match="title">
<xsl:text>,
    title = {{</xsl:text><xsl:value-of select="." /><xsl:text>}}</xsl:text>
</xsl:template>

<xsl:template match="updated">
<xsl:text>,
    year = {</xsl:text><xsl:value-of select="substring(.,1,4)" /><xsl:text>},
    month = </xsl:text>
<xsl:choose>
<xsl:when test="substring(.,6,2) = '01'"><xsl:text>jan</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '02'"><xsl:text>feb</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '03'"><xsl:text>mar</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '04'"><xsl:text>apr</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '05'"><xsl:text>may</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '06'"><xsl:text>jun</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '07'"><xsl:text>jul</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '08'"><xsl:text>aug</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '09'"><xsl:text>sep</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '10'"><xsl:text>oct</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '11'"><xsl:text>nov</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '12'"><xsl:text>dec</xsl:text></xsl:when>
<xsl:otherwise>{}</xsl:otherwise></xsl:choose>
</xsl:template>

<xsl:template match="summary">
<xsl:text>,
    abstract = {</xsl:text><xsl:value-of select="." /><xsl:text>}</xsl:text>
</xsl:template>

<xsl:template match="arxiv:doi">
<xsl:text>,
    doi = {</xsl:text><xsl:value-of select="." /><xsl:text>}</xsl:text>
</xsl:template>

<xsl:template match="arxiv:journal_ref">
<!-- FIXME split journal_ref into journal name, volume, issue, year, ... -->
<xsl:text>,
    journal = {</xsl:text><xsl:value-of select="." /><xsl:text>}</xsl:text>
</xsl:template>



</xsl:stylesheet>
