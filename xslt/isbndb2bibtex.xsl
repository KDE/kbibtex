<xsl:transform version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

<!--
  - This Extensible Stylesheet Language Transformation file translates XML files
  - as provided by ISBNdb.com into BibTeX files.
  -
  - This file was written by Thomas Fischer <fischer@unix-ag.uni-kl.de>
  - It is released under the GNU Public License version 2 or later.
  -
  -->

<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>
<xsl:strip-space elements="*"/>


<!-- START HERE -->
<xsl:template match="/">
<!-- process each entry -->
<xsl:apply-templates select="ISBNdb/BookList/BookData" />
</xsl:template>


<xsl:template match="BookData">
<!-- opening entry -->
<xsl:text>@book{</xsl:text>
<xsl:value-of select="@book_id" />

<!-- title -->
<xsl:choose>
<xsl:when test="TitleLong and string-length(TitleLong)>2">
<xsl:text>,
    title = {{</xsl:text>
<xsl:value-of select="TitleLong" />
<xsl:text>}}</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:if test="Title and string-length(Title)>2">
<xsl:text>,
    title = {{</xsl:text>
<xsl:value-of select="Title" />
<xsl:text>}}</xsl:text>
</xsl:if>
</xsl:otherwise>
</xsl:choose>

<!-- summary/abstract -->
<xsl:if test="Summary and string-length(Summary)>2">
<xsl:text>,
    abstract = {</xsl:text>
<xsl:value-of select="Summary" />
<xsl:text>}</xsl:text>
</xsl:if>

<!-- publisher -->
<xsl:if test="PublisherText and string-length(PublisherText)>2">
<xsl:text>,
    publisher = {</xsl:text>
<xsl:value-of select="PublisherText" />
<xsl:text>}</xsl:text>
</xsl:if>

<!-- authors (no detailed parsing here) -->
<xsl:if test="AuthorsText and string-length(AuthorsText)>2">
<xsl:text>,
    author = {{</xsl:text>
<xsl:value-of select="AuthorsText" />
<xsl:text>}}</xsl:text>
</xsl:if>

<!-- closing entry -->
<xsl:text>
}

</xsl:text>
</xsl:template>

</xsl:transform>
