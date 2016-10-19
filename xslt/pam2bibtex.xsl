<xsl:transform version='2.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform' xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:pam="http://prismstandard.org/namespaces/pam/2.0/" xmlns:prism="http://prismstandard.org/namespaces/basic/2.0/" xmlns:xhtml="http://www.w3.org/1999/xhtml">

<!--
  - This Extensible Stylesheet Language Transformation file translates XML files
  - as provided by Springer in the format PAM (PRISM Aggregator Message)
  - into BibTeX files.
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
<xsl:apply-templates select="response/records/pam:message" />
</xsl:template>

<xsl:template match="pam:message">
<xsl:apply-templates select="xhtml:head/pam:article" />

<!-- abstract -->
<xsl:if test="xhtml:body/p">
<xsl:text>,
    abstract = {</xsl:text>
<xsl:for-each select="xhtml:body/p">
<xsl:apply-templates select="."/>
<xsl:if test="position()!=last()"><xsl:text>
</xsl:text></xsl:if>
</xsl:for-each>
<xsl:text>}</xsl:text>
</xsl:if>

<!-- closing entry -->
<xsl:text>
}

</xsl:text>
</xsl:template>


<xsl:template match="pam:article">

<!-- entry Type -->
<xsl:choose>
<xsl:when test="journalId">
<xsl:text>@article{</xsl:text>
</xsl:when>
<xsl:when test="dc:title and prism:publicationName and ( prism:isbn or printIsbn or electronicIsbn)">
<!-- an article in a book has a title and a book title and the book has an ISBN -->
<xsl:text>@inbook{</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>@misc{</xsl:text>
</xsl:otherwise>
</xsl:choose>

<!-- identifier -->
<xsl:text>PAM_</xsl:text><xsl:value-of select="dc:identifier" />

<!-- title -->
<xsl:if test="dc:title">
<xsl:text>,
    title = {{</xsl:text>
<xsl:value-of select="dc:title" />
<xsl:text>}}</xsl:text>
</xsl:if>

<!-- author -->
<xsl:if test="dc:creator">
<xsl:text>,
    author = {</xsl:text>
<xsl:for-each select="dc:creator">
<xsl:apply-templates select="."/>
<xsl:if test="position()!=last()"><xsl:text> and </xsl:text></xsl:if>
</xsl:for-each>
<xsl:text>}</xsl:text>
</xsl:if>

<!-- journal -->
<xsl:if test="journalId and prism:publicationName">
<xsl:text>,
    journal = {{</xsl:text>
<xsl:value-of select="prism:publicationName" />
<xsl:text>}}</xsl:text>
</xsl:if>

<!-- book title -->
<xsl:if test="dc:title and prism:publicationName and ( prism:isbn or printIsbn or electronicIsbn)">
<xsl:text>,
    booktitle = {{</xsl:text>
<xsl:value-of select="prism:publicationName" />
<xsl:text>}}</xsl:text>
</xsl:if>

<!-- ISSN -->
<xsl:if test="prism:issn">
<xsl:text>,
    issn = {</xsl:text>
<xsl:value-of select="prism:issn" />
<xsl:text>}</xsl:text>
</xsl:if>

<!-- ISBN -->
<xsl:choose>
<xsl:when test="prism:isbn">
<xsl:text>,
    isbn = {</xsl:text>
<xsl:value-of select="prism:isbn" />
<xsl:text>}</xsl:text>
</xsl:when>
<xsl:when test="printIsbn">
<xsl:text>,
    isbn = {</xsl:text>
<xsl:value-of select="printIsbn" />
<xsl:text>}</xsl:text>
</xsl:when>
<xsl:when test="electronicIsbn">
<xsl:text>,
    isbn = {</xsl:text>
<xsl:value-of select="electronicIsbn" />
<xsl:text>}</xsl:text>
</xsl:when>
</xsl:choose>

<!-- DOI -->
<xsl:if test="prism:doi">
<xsl:text>,
    doi = {</xsl:text>
<xsl:value-of select="prism:doi" />
<xsl:text>}</xsl:text>
</xsl:if>

<!-- URL -->
<xsl:if test="prism:url">
<xsl:text>,
    url = {</xsl:text>
<xsl:value-of select="prism:url" />
<xsl:text>}</xsl:text>
</xsl:if>

<!-- publisher -->
<xsl:if test="dc:publisher">
<xsl:text>,
    publisher = {</xsl:text>
<xsl:value-of select="dc:publisher" />
<xsl:text>}</xsl:text>
</xsl:if>

<!-- volume -->
<xsl:if test="prism:volume">
<xsl:text>,
    volume = {</xsl:text>
<xsl:value-of select="prism:volume" />
<xsl:text>}</xsl:text>
</xsl:if>

<!-- number/issue -->
<xsl:if test="prism:number">
<xsl:text>,
    number = {</xsl:text>
<xsl:value-of select="prism:number" />
<xsl:text>}</xsl:text>
</xsl:if>

<!-- pages -->
<xsl:if test="prism:startingPage">
<xsl:text>,
    pages = {</xsl:text>
<xsl:value-of select="prism:startingPage" />
<xsl:if test="prism:endingPage">
<xsl:text>--</xsl:text>
<xsl:value-of select="prism:endingPage" />
</xsl:if>
<xsl:text>}</xsl:text>
</xsl:if>

<!-- year -->
<xsl:if test="prism:publicationDate">
<xsl:text>,
    year = {</xsl:text>
<xsl:value-of select="substring(prism:publicationDate,1,4)" />
<xsl:text>}</xsl:text>
</xsl:if>

</xsl:template>

</xsl:transform>
