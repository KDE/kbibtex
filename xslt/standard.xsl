<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output encoding="UTF-8"/>

<!-- ==============================================================================
     Maintain original HTML tags
-->
<xsl:template match="a|abbr|acronym|address|applet|b|big|blockquote|br|cite|code|del|dfn|div|em|hr|i|kbd|p|param|pre|q|quote|samp|script|span|small|strike|strong|sub|sup|tt|var|button|fieldset|form|input|label|legend|object|option|optgroup|select|caption|col|colgroup|table|tbody|td|tfoot|th|thead|tr|dl|dd|dt|ol|ul|li|img|quote|quotation" xmlns:html="http://www.w3.org/1999/XSL/some">
<xsl:copy>
<xsl:copy-of select="@*" />
<xsl:apply-templates />
</xsl:copy>
</xsl:template>



<xsl:template match="text">
<xsl:apply-templates />
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
<xsl:text>, Eds: </xsl:text>
<xsl:for-each select="person">
<xsl:apply-templates select="."/><xsl:if test="position()!=last()"><xsl:text>, </xsl:text>
</xsl:if>
<xsl:if test="position()=last()-1"><xsl:text> and </xsl:text></xsl:if></xsl:for-each>
</xsl:template>

<xsl:template match="title">
<b><xsl:apply-templates /></b>
</xsl:template>

<xsl:template match="booktitle">
<xsl:text>, </xsl:text><i><xsl:apply-templates /></i>
</xsl:template>

<xsl:template match="school">
<xsl:text>, </xsl:text><xsl:apply-templates />
</xsl:template>

<xsl:template match="journal">
<xsl:text>, </xsl:text>
<i><xsl:apply-templates /></i>
<xsl:if test="string-length(../volume)>0">
<xsl:text> </xsl:text>
<xsl:value-of select="../volume"/>
<xsl:if test="string-length(../number)>0">
<xsl:text>(</xsl:text>
<xsl:value-of select="../number"/>
<xsl:text>)</xsl:text>
</xsl:if>
</xsl:if>
</xsl:template>

<xsl:template match="institution">
<xsl:text>, </xsl:text>
<i><xsl:apply-templates /></i>
<xsl:if test="string-length(../number)>0">
<xsl:text> No. </xsl:text>
<xsl:value-of select="../number"/>
</xsl:if>
</xsl:template>

<xsl:template match="publisher">
<xsl:text>, </xsl:text>
<xsl:apply-templates />
</xsl:template>

<xsl:template match="volume">
<xsl:if test="string-length(../journal)=0">
<!-- do not print volume if there is "journal" field,
     which prints the volume, too.                     -->
<xsl:text>, </xsl:text>
<xsl:text>volume </xsl:text>
<xsl:apply-templates />
</xsl:if>
</xsl:template>

<xsl:template match="edition">
<xsl:text>, </xsl:text>
<xsl:apply-templates />
<xsl:text> edition</xsl:text>
</xsl:template>

<xsl:template match="pages">
<xsl:text>, </xsl:text>
<xsl:apply-templates />
</xsl:template>

<xsl:template match="year">
<xsl:text>, </xsl:text>
<xsl:if test="string-length(../month)>0">
<xsl:value-of select="../month"/>
<xsl:text> </xsl:text>
</xsl:if>
<xsl:apply-templates />
</xsl:template>

<xsl:template match="note">
<xsl:text>, </xsl:text><xsl:apply-templates />
</xsl:template>

<xsl:template match="abstract">
<br/><small><i>Abstract</i><xsl:text>: </xsl:text><xsl:apply-templates /></small>
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
