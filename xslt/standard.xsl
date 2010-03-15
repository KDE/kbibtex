<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output encoding="UTF-8"/>

<xsl:template match="text">
<xsl:value-of select="."/>
</xsl:template>

<xsl:template match="person">
<xsl:value-of select="lastname"/><xsl:if test="string-length(firstname)>0"><xsl:text>, </xsl:text>
<xsl:value-of select="firstname"/></xsl:if>
</xsl:template>

<xsl:template match="authors">
<xsl:for-each select="person">
<xsl:apply-templates select="."/><xsl:if test="position()!=last()"><xsl:text>, </xsl:text>
</xsl:if>
<xsl:if test="position()=last()-1">
<xsl:text> and </xsl:text>
</xsl:if>
</xsl:for-each>
<xsl:text>: </xsl:text>
</xsl:template>

<xsl:template match="editors">
<xsl:text>Eds: </xsl:text>
<xsl:for-each select="person">
<xsl:apply-templates select="."/><xsl:if test="position()!=last()"><xsl:text>, </xsl:text>
</xsl:if>
<xsl:if test="position()=last()-1"><xsl:text> and </xsl:text></xsl:if></xsl:for-each><xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="title">
<b><xsl:value-of select="."/></b><xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="booktitle">
<i><xsl:value-of select="."/></i>
<xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="school">
<xsl:value-of select="."/>
<xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="journal">
<i><xsl:value-of select="."/></i>
<xsl:if test="string-length(../volume)>0">
<xsl:text> </xsl:text>
<xsl:value-of select="../volume"/>
<xsl:if test="string-length(../number)>0">
<xsl:text>(</xsl:text>
<xsl:value-of select="../number"/>
<xsl:text>)</xsl:text>
</xsl:if>
</xsl:if>
<xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="institution">
<i><xsl:value-of select="."/></i>
<xsl:if test="string-length(../number)>0">
<xsl:text> No. </xsl:text>
<xsl:value-of select="../number"/>
</xsl:if>
<xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="publisher">
<xsl:value-of select="."/>
<xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="volume">
<xsl:text>volume </xsl:text>
<xsl:value-of select="."/>
<xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="edition">
<xsl:value-of select="."/>
<xsl:text> edition, </xsl:text>
</xsl:template>

<xsl:template match="pages">
<xsl:value-of select="."/><xsl:text>, </xsl:text>
</xsl:template>

<xsl:template match="year">
<xsl:value-of select="."/>
</xsl:template>

<xsl:template match="note">
<xsl:text>, </xsl:text><xsl:value-of select="."/>
</xsl:template>

<xsl:template match="month">
<xsl:value-of select="."/><xsl:text> </xsl:text>
</xsl:template>

<xsl:template match="abstract">
<br/><i>Abstract</i><xsl:text>: </xsl:text><xsl:value-of select="."/>
</xsl:template>

<xsl:template match="entry">
<p>
<xsl:apply-templates select="authors" />
<xsl:apply-templates select="title" />
<xsl:apply-templates select="booktitle" />
<xsl:apply-templates select="journal" />
<xsl:apply-templates select="school" />
<xsl:apply-templates select="volume" />
<xsl:apply-templates select="edition" />
<xsl:apply-templates select="publisher" />
<xsl:apply-templates select="institution" />
<xsl:apply-templates select="pages" />
<xsl:apply-templates select="editors" />
<xsl:apply-templates select="month" />
<xsl:apply-templates select="year" />
<xsl:apply-templates select="note" />
<xsl:apply-templates select="abstract" />
</p>
</xsl:template>

<xsl:template match="bibliography">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
<title>Bibliography</title>
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
</head>
<body>
<xsl:apply-templates select="entry" />
</body>
</html>
</xsl:template>

</xsl:stylesheet>
