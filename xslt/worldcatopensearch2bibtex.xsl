<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform' xmlns:oclcterms="http://purl.org/oclc/terms/" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:opensearch="http://a9.com/-/spec/opensearch/1.1/">

<!--
  - This Extensible Stylesheet Language Transformation file translates XML files
  - as provided by WorldCat's OpenSearch into BibTeX files.
  -
  - This file was written by Thomas Fischer <fischer@unix-ag.uni-kl.de>
  - It is released under the GNU Public License version 2 or later.
  -
  - To run test this transformation file, run e.g.
  - wget "http://www.worldcat.org/webservices/catalog/search/worldcat/opensearch?q=APIs&format=atom&count=3&wskey=YOUR-API-KEY" -O | sed -e 's/xmlns="http:\/\/www.w3.org\/2005\/Atom"//' | xsltproc worldcatopensearch2bibtex.xsl -
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
<xsl:text>@book{</xsl:text>
<!-- guess a good identifier -->
<xsl:choose>
<xsl:when test="substring(id,1,25)='http://worldcat.org/oclc/'">
<xsl:value-of select='substring(id,21,100)' />
</xsl:when>
<xsl:when test="oclcterms:recordIdentifier">
<xsl:text>oclc/</xsl:text>
<xsl:value-of select='oclcterms:recordIdentifier' />
</xsl:when>
<xsl:otherwise>
<xsl:text>unknown-from-worldcat-oclc</xsl:text>
</xsl:otherwise>
</xsl:choose>

<!-- process authors by merging all names with "and" -->
<xsl:text>,
    author = {</xsl:text>
<xsl:for-each select="author/name" >
<xsl:if test="position() > 1"><xsl:text> and </xsl:text></xsl:if>
<xsl:value-of select="."/>
</xsl:for-each>
<xsl:text>}</xsl:text>

<xsl:apply-templates select="title" />
<xsl:apply-templates select="summary" />
<xsl:apply-templates select="link" />
<xsl:apply-templates select="dc:identifier" />
<xsl:apply-templates select="oclcterms:recordIdentifier" />
<xsl:text>
}

</xsl:text>
</xsl:template>


<xsl:template match="title">
<xsl:text>,
    title = {{</xsl:text><xsl:value-of select="." /><xsl:text>}}</xsl:text>
</xsl:template>

<xsl:template match="summary">
<xsl:text>,
    abstract = {</xsl:text><xsl:value-of select="." /><xsl:text>}</xsl:text>
</xsl:template>

<xsl:template match="link">
<xsl:text>,
    url = {</xsl:text><xsl:value-of select="@href" /><xsl:text>}</xsl:text>
</xsl:template>

<xsl:template match="dc:identifier">
<xsl:if test="substring(.,0,10)='urn:ISBN:'">
<xsl:text>,
    isbn = {</xsl:text><xsl:value-of select="substring(.,10,20)" /><xsl:text>}</xsl:text>
</xsl:if>
</xsl:template>

<xsl:template match="oclcterms:recordIdentifier">
<xsl:text>,
    oclc = {</xsl:text><xsl:value-of select="." /><xsl:text>}</xsl:text>
</xsl:template>

</xsl:stylesheet>
