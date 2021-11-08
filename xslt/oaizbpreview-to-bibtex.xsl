<xsl:stylesheet version='2.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform' xmlns:oai="http://www.openarchives.org/OAI/2.0/" xmlns:oai_zb_preview="https://zbmath.org/OAI/2.0/oai_zb_preview/" xmlns:zbmath="https://zbmath.org/zbmath/elements/1.0/">

<!--
  - This Extensible Stylesheet Language Transformation file translates XML files
  - as provided through zbMATH Open OAI-PMH API available at
  - https://oai.zbmath.org/
  -
  - SPDX-FileCopyrightText: 2021 Thomas Fischer <fischer@unix-ag.uni-kl.de>
  - SPDX-License-Identifier: GPL-2.0-or-later
  -->

<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>
<xsl:strip-space elements="*"/>



<!-- START HERE -->
<xsl:template match="/oai:OAI-PMH">
<!-- process each entry/record -->
<xsl:apply-templates select="oai:ListRecords/oai:record" />
</xsl:template>

<xsl:template match="oai:record">

<xsl:if test="not(oai:metadata/oai_zb_preview:zbmath/zbmath:document_title='zbMATH Open Web Interface contents unavailable due to conflicting licenses.')">

<!-- Determine publication type, use @misc as fallback -->
<xsl:choose>
<xsl:when test="oai:metadata/oai_zb_preview:zbmath/zbmath:document_type='j'">
<xsl:text>@article</xsl:text>
</xsl:when>
<xsl:when test="oai:metadata/oai_zb_preview:zbmath/zbmath:document_type='b'">
<xsl:text>@book</xsl:text>
</xsl:when>
<xsl:otherwise>
<!-- <xsl:text>@misc</xsl:text> -->
<xsl:text>@zbmathdocumenttype</xsl:text><xsl:value-of select="oai:metadata/oai_zb_preview:zbmath/zbmath:document_type" />
</xsl:otherwise>
</xsl:choose>

<!-- Entry's identifier starts with "zbmath" followed by document id -->
<xsl:text>{zbmath</xsl:text>
<xsl:value-of select="oai:metadata/oai_zb_preview:zbmath/zbmath:document_id" />


<!-- Title -->
<xsl:if test="oai:metadata/oai_zb_preview:zbmath/zbmath:document_title"><xsl:text>,
   title={{</xsl:text><xsl:value-of select="oai:metadata/oai_zb_preview:zbmath/zbmath:document_title" /><xsl:text>}}</xsl:text>
</xsl:if>

<!-- Authors -->
<xsl:if test="oai:metadata/oai_zb_preview:zbmath/zbmath:author"><xsl:text>,
   author={</xsl:text>
   <!-- Replace author separator '; ' with ' and ' -->
   <xsl:value-of select="replace(replace(oai:metadata/oai_zb_preview:zbmath/zbmath:author,'; ',' and '),';',' and ')" />
   <xsl:text>}</xsl:text>
</xsl:if>

<!-- Journal's Title -->
<xsl:if test="oai:metadata/oai_zb_preview:zbmath/zbmath:serial/zbmath:serial_title"><xsl:text>,
   journal={{</xsl:text><xsl:value-of select="oai:metadata/oai_zb_preview:zbmath/zbmath:serial/zbmath:serial_title" /><xsl:text>}}</xsl:text>
</xsl:if>

<!-- Publisher -->
<xsl:if test="oai:metadata/oai_zb_preview:zbmath/zbmath:serial/zbmath:serial_publisher"><xsl:text>,
   publisher={</xsl:text><xsl:value-of select="oai:metadata/oai_zb_preview:zbmath/zbmath:serial/zbmath:serial_publisher" /><xsl:text>}</xsl:text>
</xsl:if>

<!-- Year -->
<xsl:if test="oai:metadata/oai_zb_preview:zbmath/zbmath:publication_year"><xsl:text>,
   year={</xsl:text><xsl:value-of select="oai:metadata/oai_zb_preview:zbmath/zbmath:publication_year" /><xsl:text>}</xsl:text>
</xsl:if>

<!-- Month -->
<xsl:choose>
<xsl:when test="substring(oai:metadata/oai_zb_preview:zbmath/zbmath:time,6,2)='01'"><xsl:text>,
   month=jan</xsl:text></xsl:when>
<xsl:when test="substring(oai:metadata/oai_zb_preview:zbmath/zbmath:time,6,2)='02'"><xsl:text>,
   month=feb</xsl:text></xsl:when>
<xsl:when test="substring(oai:metadata/oai_zb_preview:zbmath/zbmath:time,6,2)='03'"><xsl:text>,
   month=mar</xsl:text></xsl:when>
<xsl:when test="substring(oai:metadata/oai_zb_preview:zbmath/zbmath:time,6,2)='04'"><xsl:text>,
   month=apr</xsl:text></xsl:when>
<xsl:when test="substring(oai:metadata/oai_zb_preview:zbmath/zbmath:time,6,2)='05'"><xsl:text>,
   month=may</xsl:text></xsl:when>
<xsl:when test="substring(oai:metadata/oai_zb_preview:zbmath/zbmath:time,6,2)='06'"><xsl:text>,
   month=jun</xsl:text></xsl:when>
<xsl:when test="substring(oai:metadata/oai_zb_preview:zbmath/zbmath:time,6,2)='07'"><xsl:text>,
   month=jul</xsl:text></xsl:when>
<xsl:when test="substring(oai:metadata/oai_zb_preview:zbmath/zbmath:time,6,2)='08'"><xsl:text>,
   month=aug</xsl:text></xsl:when>
<xsl:when test="substring(oai:metadata/oai_zb_preview:zbmath/zbmath:time,6,2)='09'"><xsl:text>,
   month=sep</xsl:text></xsl:when>
<xsl:when test="substring(oai:metadata/oai_zb_preview:zbmath/zbmath:time,6,2)='10'"><xsl:text>,
   month=oct</xsl:text></xsl:when>
<xsl:when test="substring(oai:metadata/oai_zb_preview:zbmath/zbmath:time,6,2)='11'"><xsl:text>,
   month=nov</xsl:text></xsl:when>
<xsl:when test="substring(oai:metadata/oai_zb_preview:zbmath/zbmath:time,6,2)='12'"><xsl:text>,
   month=dec</xsl:text></xsl:when>
</xsl:choose>

<!-- Pages -->
<xsl:if test="oai:metadata/oai_zb_preview:zbmath/zbmath:pagination"><xsl:text>,
   pages={</xsl:text><xsl:value-of select="replace(oai:metadata/oai_zb_preview:zbmath/zbmath:pagination,'-','--')" /><xsl:text>}</xsl:text>
</xsl:if>

<!-- Keywords -->
<xsl:if test="oai:metadata/oai_zb_preview:zbmath/zbmath:keywords/zbmath:keyword"><xsl:text>,
   keywords={</xsl:text>
<xsl:value-of select="oai:metadata/oai_zb_preview:zbmath/zbmath:keywords/zbmath:keyword" separator="," />
<xsl:text>}</xsl:text>
</xsl:if>

<!-- DOI -->
<xsl:if test="oai:metadata/oai_zb_preview:zbmath/zbmath:doi"><xsl:text>,
   doi={</xsl:text><xsl:value-of select="oai:metadata/oai_zb_preview:zbmath/zbmath:doi" /><xsl:text>}</xsl:text>
</xsl:if>

<xsl:text>
}

</xsl:text>

</xsl:if><!-- test="not(oai:metadata/oai_zb_preview:zbmath/zbmath:document_title=='zbMATH Open Web Interface contents unavailable due to conflicting licenses.')" -->

</xsl:template><!-- match="record" -->

</xsl:stylesheet>

