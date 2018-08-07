<xsl:stylesheet version='2.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform' xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:opensearch="http://a9.com/-/spec/opensearch/1.1/" xmlns:prism="http://prismstandard.org/namespaces/basic/2.0/" xmlns:atom="http://www.w3.org/2005/Atom" xmlns:ns1="http://webservices.elsevier.com/schemas/search/fast/types/v4" xmlns:sa="http://www.elsevier.com/xml/common/struct-aff/dtd">

<!--
  - This Extensible Stylesheet Language Transformation file translates XML files
  - as provided through the ScienceDirect Search API accessible via
  - api.elsevier.com
  -
  - This file was written by Thomas Fischer <fischer@unix-ag.uni-kl.de>
  - It is released under the GNU Public License version 3 or later.
  -
  -->

<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>
<xsl:strip-space elements="*"/>


<!-- START HERE -->
<xsl:template match="/search-results">
<!-- process each entry -->
<xsl:apply-templates select="entry" />
</xsl:template>

<xsl:template match="entry[error]">
<!-- do nothing on empty result set -->
</xsl:template>

<xsl:template match="entry[not(error)]">

<!-- Determine publication type, use @misc as fallback -->
<xsl:choose>
<xsl:when test="prism:publicationName">
<xsl:text>@article</xsl:text>
</xsl:when>
<xsl:otherwise>
<xsl:text>@misc</xsl:text>
</xsl:otherwise>
</xsl:choose>

<!-- Entry's identifier starts with "ieee" followed by article number -->
<xsl:text>{sciencedirect</xsl:text>
<xsl:value-of select="replace(replace(replace(pii,'\)',''),'\(',''),'-','')" />

<!-- Journal's Title -->
<xsl:if test="prism:publicationName"><xsl:text>,
   journal={{</xsl:text><xsl:value-of select="prism:publicationName" /><xsl:text>}}</xsl:text></xsl:if>

<!-- Title -->
<xsl:if test="dc:title"><xsl:text>,
   title={{</xsl:text><xsl:value-of select="dc:title" /><xsl:text>}}</xsl:text>
</xsl:if>

<!-- Authors -->
<xsl:if test="authors/author/surname"><xsl:text>,
   author={</xsl:text>
<!-- Join all authors' full name using " and " as separator -->
<xsl:for-each select="authors/author">
<xsl:value-of select="surname" /><xsl:text>, </xsl:text><xsl:value-of select="given-name" />
<xsl:if test="position() != last()"><xsl:text> and </xsl:text></xsl:if>
</xsl:for-each>
<xsl:text>}</xsl:text>
</xsl:if>

<!-- Month -->
<xsl:choose>
<xsl:when test="substring(prism:coverDate,5,4)='-01-'"><xsl:text>,
   month=jan</xsl:text></xsl:when>
<xsl:when test="substring(prism:coverDate,5,4)='-02-'"><xsl:text>,
   month=feb</xsl:text></xsl:when>
<xsl:when test="substring(prism:coverDate,5,4)='-03-'"><xsl:text>,
   month=mar</xsl:text></xsl:when>
<xsl:when test="substring(prism:coverDate,5,4)='-04-'"><xsl:text>,
   month=apr</xsl:text></xsl:when>
<xsl:when test="substring(prism:coverDate,5,4)='-05-'"><xsl:text>,
   month=may</xsl:text></xsl:when>
<xsl:when test="substring(prism:coverDate,5,4)='-06-'"><xsl:text>,
   month=jun</xsl:text></xsl:when>
<xsl:when test="substring(prism:coverDate,5,4)='-07-'"><xsl:text>,
   month=jul</xsl:text></xsl:when>
<xsl:when test="substring(prism:coverDate,5,4)='-08-'"><xsl:text>,
   month=aug</xsl:text></xsl:when>
<xsl:when test="substring(prism:coverDate,5,4)='-09-'"><xsl:text>,
   month=sep</xsl:text></xsl:when>
<xsl:when test="substring(prism:coverDate,5,4)='-10-'"><xsl:text>,
   month=oct</xsl:text></xsl:when>
<xsl:when test="substring(prism:coverDate,5,4)='-11-'"><xsl:text>,
   month=nov</xsl:text></xsl:when>
<xsl:when test="substring(prism:coverDate,5,4)='-12-'"><xsl:text>,
   month=dec</xsl:text></xsl:when>
</xsl:choose>

<!-- Year -->
<xsl:if test="substring(prism:coverDate,1,2)='19' or substring(prism:coverDate,1,2)='20'"><xsl:text>,
   year={</xsl:text><xsl:value-of select="substring(prism:coverDate,1,4)" /><xsl:text>}</xsl:text></xsl:if>

<!-- Pages -->
<xsl:if test="prism:startingPage"><xsl:text>,
   pages={</xsl:text><xsl:value-of select="prism:startingPage" />
<xsl:if test="prism:endingPage"><xsl:text>--</xsl:text><xsl:value-of select="prism:endingPage" /></xsl:if>
<xsl:text>}</xsl:text></xsl:if>

<!-- ISSN -->
<xsl:if test="prism:issn"><xsl:text>,
   issn={</xsl:text><xsl:value-of select="prism:issn" /><xsl:text>}</xsl:text></xsl:if>

<!-- Volume -->
<xsl:if test="prism:volume"><xsl:text>,
   volume={</xsl:text><xsl:value-of select="prism:volume" /><xsl:text>}</xsl:text></xsl:if>

<!-- Number/Issue -->
<xsl:if test="prism:issueIdentifier"><xsl:text>,
   number={</xsl:text><xsl:value-of select="prism:issueIdentifier" /><xsl:text>}</xsl:text></xsl:if>

<!-- DOI -->
<xsl:if test="prism:doi"><xsl:text>,
   doi={</xsl:text><xsl:value-of select="prism:doi" /><xsl:text>}</xsl:text>
</xsl:if>

<!-- PII (ScienceDirect-specific) -->
<xsl:if test="pii"><xsl:text>,
   pii={</xsl:text><xsl:value-of select="pii" /><xsl:text>}</xsl:text>
</xsl:if>

<!-- URL-->

<xsl:if test="starts-with(link[@ref='scidir']/@href,'http')"><xsl:text>,
   url={</xsl:text><xsl:value-of select="link[@ref='scidir']/@href" /><xsl:text>}</xsl:text>
</xsl:if>


<!-- Close BibTeX entry -->
<xsl:text>
}

</xsl:text>

</xsl:template>

</xsl:stylesheet>
