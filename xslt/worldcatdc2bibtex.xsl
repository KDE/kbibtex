<xsl:transform version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform' xmlns="http://www.loc.gov/zing/srw/" xmlns:oclcterms="http://purl.org/oclc/terms/" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
<!--  xmlns:diag="http://www.loc.gov/zing/srw/diagnostic/"  -->

<!--
  - This Extensible Stylesheet Language Transformation file translates XML files
  - as provided by xxxxxxx
  - into BibTeX files.
  -
  - This file was written by Thomas Fischer <fischer@unix-ag.uni-kl.de>
  - It is released under the GNU Public License version 2 or later.
  -
  - To run test this transformation file, run e.g.
  - wget 'http://www.worldcat.org/webservices/catalog/search/worldcat/sru?wskey=YOURKEY&maximumRecords=10&query=srw.au+all+"smith"&recordSchema=info%3Asrw%2Fschema%2F1%2Fdc' -O - | sed -e 's/searchRetrieveResponse xmlns="http:\/\/www.loc.gov\/zing\/srw\/"/searchRetrieveResponse/' | xsltproc wordcatdc2bibtex.xsl -
  -
  -->

<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>
<xsl:strip-space elements="*"/>


<!-- START HERE -->
<xsl:template match="/">
<!-- process each entry -->
<xsl:apply-templates select="searchRetrieveResponse/records/record/recordData/oclcdcs" />
</xsl:template>


<xsl:template match="oclcdcs">
<xsl:choose>
<!-- at least identifier looks like a ISBN-13 number -->
<xsl:when test="dc:identifier[starts-with(., '978')]">
<!-- ISBN means book -->
<xsl:text>@book{</xsl:text>
</xsl:when>
<xsl:otherwise>
<!-- fallback: some other/unknown type of publication -->
<xsl:text>@misc{</xsl:text>
</xsl:otherwise>
</xsl:choose>

<!-- identifier -->
<!-- use (number) OCLC identifier for entry id -->
<xsl:text>OCLC_</xsl:text><xsl:value-of select="oclcterms:recordIdentifier[not(@xsi:type)]" />
<!-- <xsl:text>OCLC_</xsl:text><xsl:value-of select="oclcterms:recordIdentifier" />-->

<!-- title -->
<xsl:if test="dc:title">
<xsl:text>,
    title = {{</xsl:text>
<xsl:value-of select="dc:title" />
<xsl:text>}}</xsl:text>
</xsl:if>

<!-- author (a.k.a. creator) -->
<xsl:if test="dc:creator">
<xsl:text>,
    author = {</xsl:text>
<xsl:for-each select="dc:creator">
<xsl:apply-templates select="."/>
<!-- separate multiple authors/creators by " and " -->
<xsl:if test="position()!=last()"><xsl:text> and </xsl:text></xsl:if>
</xsl:for-each>
<xsl:text>}</xsl:text>
</xsl:if>

<!-- ISBN -->
<!-- if generic "identifier" tag's content starts with "978", assume ISBN -->
<!-- TODO filter for duplicates, remove text and comments -->
<xsl:for-each select="dc:identifier[starts-with(., '978')]">
<xsl:text>,
    isbn = {</xsl:text>
<xsl:value-of select="." />
<xsl:text>}</xsl:text>
</xsl:for-each>

<!-- URL -->
<!-- if generic "identifier" tag's content starts with "http", assume URL -->
<xsl:if test="dc:identifier[starts-with(., 'http')]">
<xsl:text>,
    url = {</xsl:text>
<xsl:for-each select="dc:identifier[starts-with(., 'http')]">
<xsl:apply-templates select="."/>
<!-- separate URLs by "; " -->
<xsl:if test="position()!=last()"><xsl:text>; </xsl:text></xsl:if>
</xsl:for-each>
<xsl:text>}</xsl:text>
</xsl:if>

<!-- publisher -->
<xsl:if test="dc:publisher">
<xsl:text>,
    publisher = {</xsl:text>
<xsl:value-of select="dc:publisher" />
<xsl:text>}</xsl:text>
</xsl:if>

<!-- year -->
<xsl:if test="dc:date">
<xsl:text>,
    year = {</xsl:text>
<!-- TODO remove text and comments -->
<xsl:value-of select="dc:date" />
<xsl:text>}</xsl:text>
</xsl:if>

<!-- keywords -->
<!-- TODO difference keyword <-> subject? -->
<xsl:if test="dc:subject">
<xsl:text>,
    keywords = {</xsl:text>
<xsl:for-each select="dc:subject">
<xsl:apply-templates select="."/>
<!-- separate keywords by "; " -->
<!-- TODO some additional filtering? -->
<xsl:if test="position()!=last()"><xsl:text>; </xsl:text></xsl:if>
</xsl:for-each>
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:text>
}

</xsl:text>

</xsl:template>

</xsl:transform>
