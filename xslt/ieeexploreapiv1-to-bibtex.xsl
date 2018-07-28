<xsl:stylesheet version='2.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

<!--
  - This Extensible Stylesheet Language Transformation file translates XML files
  - as provided through IEEE Xplore's Metadata Search API v1 accessible via
  - https://ieeexploreapi.ieee.org/api/v1/search/articles
  -
  - This file was written by Thomas Fischer <fischer@unix-ag.uni-kl.de>
  - It is released under the GNU Public License version 3 or later.
  -
  -->

<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>
<xsl:strip-space elements="*"/>


<!-- START HERE -->
<xsl:template match="/articles">
<!-- process each entry -->
<xsl:apply-templates select="article" />
</xsl:template>

<xsl:template match="article">

<!-- Determine publication type, use @misc as fallback -->
<xsl:choose>
<xsl:when test="content_type='Conferences'">
<xsl:text>@inproceedings</xsl:text>
</xsl:when>
<xsl:when test="content_type='Journals'">
<xsl:text>@article</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>@misc</xsl:text>
</xsl:otherwise>
</xsl:choose>

<!-- Entry's identifier starts with "ieee" followed by article number -->
<xsl:text>{ieee</xsl:text>
<xsl:value-of select="article_number" />


<!-- Title -->
<xsl:if test="title"><xsl:text>,
   title={{</xsl:text><xsl:value-of select="title" /><xsl:text>}}</xsl:text>
</xsl:if>

<!-- Authors -->
<xsl:if test="authors/author/full_name"><xsl:text>,
   author={</xsl:text>
<!-- Join all authors' full name using " and " as separator -->
<xsl:value-of select="authors/author/full_name" separator=" and " />
<xsl:text>}</xsl:text>
</xsl:if>

<!-- Conference's Title -->
<xsl:if test="publication_title and content_type='Conferences'"><xsl:text>,
   booktitle={{</xsl:text><xsl:value-of select="publication_title" /><xsl:text>}}</xsl:text></xsl:if>

<!-- Journal's Title -->
<xsl:if test="publication_title and content_type='Journals'"><xsl:text>,
   journal={{</xsl:text><xsl:value-of select="publication_title" /><xsl:text>}}</xsl:text></xsl:if>

<!-- Publisher -->
<xsl:if test="publisher"><xsl:text>,
   publisher={</xsl:text><xsl:value-of select="publisher" /><xsl:text>}</xsl:text></xsl:if>

<!-- Abstract -->
<xsl:if test="abstract"><xsl:text>,
   abstract={</xsl:text><xsl:value-of select="abstract" /><xsl:text>}</xsl:text></xsl:if>

<!-- Month -->
<xsl:choose>
<xsl:when test="contains(conference_dates,' Jan') or starts-with(conference_dates,'Jan') or contains(publication_date,' Jan') or starts-with(publication_date,'Jan')"><xsl:text>,
   month=jan</xsl:text></xsl:when>
<xsl:when test="contains(conference_dates,' Feb') or starts-with(conference_dates,'Feb') or contains(publication_date,' Feb') or starts-with(publication_date,'Feb')"><xsl:text>,
   month=feb</xsl:text></xsl:when>
<xsl:when test="contains(conference_dates,' Mar') or starts-with(conference_dates,'Mar') or contains(publication_date,' Mar') or starts-with(publication_date,'Mar')"><xsl:text>,
   month=mar</xsl:text></xsl:when>
<xsl:when test="contains(conference_dates,' Apr') or starts-with(conference_dates,'Apr') or contains(publication_date,' Apr') or starts-with(publication_date,'Apr')"><xsl:text>,
   month=apr</xsl:text></xsl:when>
<xsl:when test="contains(conference_dates,' May') or starts-with(conference_dates,'May') or contains(publication_date,' May') or starts-with(publication_date,'May')"><xsl:text>,
   month=may</xsl:text></xsl:when>
<xsl:when test="contains(conference_dates,' Jun') or starts-with(conference_dates,'Jun') or contains(publication_date,' Jun') or starts-with(publication_date,'Jun')"><xsl:text>,
   month=jun</xsl:text></xsl:when>
<xsl:when test="contains(conference_dates,' Jul') or starts-with(conference_dates,'Jul') or contains(publication_date,' Jul') or starts-with(publication_date,'Jul')"><xsl:text>,
   month=jul</xsl:text></xsl:when>
<xsl:when test="contains(conference_dates,' Aug') or starts-with(conference_dates,'Aug') or contains(publication_date,' Aug') or starts-with(publication_date,'Aug')"><xsl:text>,
   month=aug</xsl:text></xsl:when>
<xsl:when test="contains(conference_dates,' Sep') or starts-with(conference_dates,'Sep') or contains(publication_date,' Sep') or starts-with(publication_date,'Sep')"><xsl:text>,
   month=sep</xsl:text></xsl:when>
<xsl:when test="contains(conference_dates,' Oct') or starts-with(conference_dates,'Oct') or contains(publication_date,' Oct') or starts-with(publication_date,'Oct')"><xsl:text>,
   month=oct</xsl:text></xsl:when>
<xsl:when test="contains(conference_dates,' Nov') or starts-with(conference_dates,'Nov') or contains(publication_date,' Nov') or starts-with(publication_date,'Nov')"><xsl:text>,
   month=nov</xsl:text></xsl:when>
<xsl:when test="contains(conference_dates,' Dec') or starts-with(conference_dates,'Dec') or contains(publication_date,' Dec') or starts-with(publication_date,'Dec')"><xsl:text>,
   month=dec</xsl:text></xsl:when>
</xsl:choose>

<!-- Year -->
<xsl:if test="publication_date"><xsl:text>,
   year={</xsl:text><xsl:value-of select="substring(publication_date,string-length(publication_date)-3)" /><xsl:text>}</xsl:text></xsl:if>
<xsl:if test="not(publication_date) and conference_dates"><xsl:text>,
   year={</xsl:text><xsl:value-of select="substring(conference_dates,string-length(conference_dates)-3)" /><xsl:text>}</xsl:text></xsl:if>

<!-- Pages -->
<xsl:if test="start_page"><xsl:text>,
   pages={</xsl:text><xsl:value-of select="start_page" />
<xsl:if test="end_page"><xsl:text>--</xsl:text><xsl:value-of select="end_page" /></xsl:if>
<xsl:text>}</xsl:text></xsl:if>

<!-- ISSN -->
<xsl:if test="issn"><xsl:text>,
   issn={</xsl:text><xsl:value-of select="issn" /><xsl:text>}</xsl:text></xsl:if>

<!-- ISBN -->
<xsl:if test="isbn"><xsl:text>,
   isbn={</xsl:text><xsl:value-of select="replace(isbn,'New-2005_Electronic_','')" /><xsl:text>}</xsl:text></xsl:if>

<!-- Number/Issue -->
<xsl:if test="issue"><xsl:text>,
   number={</xsl:text><xsl:value-of select="issue" /><xsl:text>}</xsl:text></xsl:if>

<!-- Volume -->
<xsl:if test="volume"><xsl:text>,
   volume={</xsl:text><xsl:value-of select="volume" /><xsl:text>}</xsl:text></xsl:if>

<!-- keywords -->
<xsl:if test="index_terms/*/term"><xsl:text>,
   keywords={</xsl:text>
<xsl:value-of select="index_terms/*/term" separator="," />
<xsl:text>}</xsl:text>
</xsl:if>

<!-- DOI -->
<xsl:if test="doi"><xsl:text>,
   doi={</xsl:text><xsl:value-of select="doi" /><xsl:text>}</xsl:text></xsl:if>

<!-- PDF and URLs-->
<xsl:if test="pdf_url or abstract_url"><xsl:text>,
   url={</xsl:text>
   <xsl:if test="pdf_url"><xsl:value-of select="pdf_url" /></xsl:if>
   <xsl:if test="pdf_url and abstract_url"><xsl:text>;</xsl:text></xsl:if>
   <xsl:if test="abstract_url"><xsl:value-of select="abstract_url" /></xsl:if>
<xsl:text>}</xsl:text></xsl:if>

<!-- Close BibTeX entry -->
<xsl:text>
}

</xsl:text>

</xsl:template>

</xsl:stylesheet>
