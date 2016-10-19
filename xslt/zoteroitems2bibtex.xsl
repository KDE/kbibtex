<xsl:transform version='2.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform' xmlns:zapi="http://zotero.org/ns/api"  xmlns:atom="http://www.w3.org/2005/Atom" xmlns:xhtml="http://www.w3.org/1999/xhtml">

<!--
  - This Extensible Stylesheet Language Transformation file translates XML files
  - as provided by Zotero (items, not collections) into BibTeX files.
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
<xsl:apply-templates select="atom:feed/atom:entry" />
</xsl:template>

<xsl:template match="atom:entry">

<xsl:choose>
<xsl:when test="zapi:itemType='book'">
<xsl:text>@book</xsl:text>
</xsl:when>
<xsl:when test="zapi:itemType='journalArticle'">
<xsl:text>@article</xsl:text>
</xsl:when>
<xsl:when test="zapi:itemType='conferencePaper'">
<xsl:text>@inproceedings</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>@misc</xsl:text>
</xsl:otherwise>
</xsl:choose>

<xsl:text>{</xsl:text><xsl:value-of select="zapi:key" />

<xsl:if test="atom:title">
<xsl:text>,
title = {{</xsl:text>
<xsl:value-of select="atom:title" />
<xsl:text>}}</xsl:text>
</xsl:if>

<xsl:if test="zapi:year">
<xsl:text>,
year = {</xsl:text>
<xsl:value-of select="zapi:year" />
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='volume' and xhtml:th='Volume']/xhtml:td">
<xsl:text>,
volume = {</xsl:text>
<xsl:value-of select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='volume' and xhtml:th='Volume']/xhtml:td" />
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='issue' and xhtml:th='Issue']/xhtml:td">
<xsl:text>,
number = {</xsl:text>
<xsl:value-of select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='issue' and xhtml:th='Issue']/xhtml:td" />
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='pages' and xhtml:th='Pages']/xhtml:td">
<xsl:text>,
pages = {</xsl:text>
<xsl:value-of select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='pages' and xhtml:th='Pages']/xhtml:td" />
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='series' and xhtml:th='Series']/xhtml:td">
<xsl:text>,
series = {</xsl:text>
<xsl:value-of select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='series' and xhtml:th='Series']/xhtml:td" />
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='proceedingsTitle' and xhtml:th='Proceedings Title']/xhtml:td">
<xsl:text>,
booktitle = {</xsl:text>
<xsl:value-of select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='proceedingsTitle' and xhtml:th='Proceedings Title']/xhtml:td" />
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='publicationTitle' and xhtml:th='Publication']/xhtml:td">
<xsl:text>,
journal = {</xsl:text>
<xsl:value-of select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='publicationTitle' and xhtml:th='Publication']/xhtml:td" />
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='ISSN' and xhtml:th='ISSN']/xhtml:td">
<xsl:text>,
issn = {</xsl:text>
<xsl:value-of select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='ISSN' and xhtml:th='ISSN']/xhtml:td" />
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='ISBN' and xhtml:th='ISBN']/xhtml:td">
<xsl:text>,
isbn = {</xsl:text>
<xsl:value-of select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='ISBN' and xhtml:th='ISBN']/xhtml:td" />
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='abstractNote' and xhtml:th='Abstract']/xhtml:td">
<xsl:text>,
abstract = {</xsl:text>
<xsl:value-of select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='abstractNote' and xhtml:th='Abstract']/xhtml:td" />
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='publisher' and xhtml:th='Publisher']/xhtml:td">
<xsl:text>,
publisher = {</xsl:text>
<xsl:value-of select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='publisher' and xhtml:th='Publisher']/xhtml:td" />
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='creator']/xhtml:th='Author'">
<xsl:text>,
author = {</xsl:text>
<xsl:for-each select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='creator' and xhtml:th='Author']/xhtml:td">
<xsl:apply-templates select="."/>
<xsl:if test="position()!=last()"><xsl:text> and </xsl:text></xsl:if>
</xsl:for-each>
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='creator']/xhtml:th='Editor'">
<xsl:text>,
editor = {</xsl:text>
<xsl:for-each select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='creator' and xhtml:th='Editor']/xhtml:td">
<xsl:apply-templates select="."/>
<xsl:if test="position()!=last()"><xsl:text> and </xsl:text></xsl:if>
</xsl:for-each>
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='url']/xhtml:th='URL'">
<xsl:text>,
url = {</xsl:text>
<xsl:for-each select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='url' and xhtml:th='URL']/xhtml:td">
<xsl:apply-templates select="."/>
<xsl:if test="position()!=last()"><xsl:text>; </xsl:text></xsl:if>
</xsl:for-each>
<xsl:text>}</xsl:text>
</xsl:if>

<xsl:if test="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='DOI']/xhtml:th='DOI'">
<xsl:text>,
doi = {</xsl:text>
<xsl:for-each select="atom:content/xhtml:div/xhtml:table/xhtml:tr[@class='DOI' and xhtml:th='DOI']/xhtml:td">
<xsl:apply-templates select="."/>
<xsl:if test="position()!=last()"><xsl:text>; </xsl:text></xsl:if>
</xsl:for-each>
<xsl:text>}</xsl:text>
</xsl:if>


<xsl:text>
}

</xsl:text>
</xsl:template>

</xsl:transform>
