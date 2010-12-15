<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
xmlns:rng="http://relaxng.org/ns/structure/1.0"
xmlns:rha="http://redhat.com/~pkennedy/annotation_namespace/cluster_conf_annot_namespace">
<xsl:output method="text" indent="yes"/>

<xsl:template match="rng:attribute">
&lt;li&gt;
&lt;b&gt;<xsl:value-of select="@name" />&lt;/b&gt; - <xsl:value-of select="normalize-space(@rha:description)"/>
&lt;/li&gt;
</xsl:template>

<xsl:template match="rng:optional">
<xsl:apply-templates select="rng:attribute" />
<xsl:apply-templates select="rng:element" />
<xsl:apply-templates select="rng:interleave" />
<xsl:apply-templates select="rng:zeroOrMore" />
<xsl:apply-templates select="rng:oneOrMore" />
</xsl:template>

<xsl:template match="rng:element" name="elem">
&lt;li&gt;
<xsl:value-of select="@name"/> - <xsl:value-of select="normalize-space(@rha:description)"/>
&lt;br/&gt;
Required Attributes:
	&lt;br/&gt;
 	&lt;ul&gt;
	<xsl:apply-templates select="rng:attribute" />
 	&lt;/ul&gt;
Optional Attributes:
	&lt;br/&gt;
 	&lt;ul&gt;
	<xsl:apply-templates select="rng:optional" />
 	&lt;/ul&gt;
Children:
	&lt;br/&gt;
 	&lt;ul&gt;
	<xsl:apply-templates select="rng:interleave" />
	<xsl:apply-templates select="rng:zeroOrMore" />
	<xsl:apply-templates select="rng:oneOrMore" />
 	&lt;/ul&gt;
&lt;/li&gt;
</xsl:template>

<xsl:template match="rng:interleave">
	<xsl:apply-templates select="child::*" />
</xsl:template>

<xsl:template match="rng:zeroOrMore">
	<xsl:apply-templates select="child::*" />
</xsl:template>

<xsl:template match="rng:oneOrMore">
	<xsl:apply-templates select="child::*" />
</xsl:template>

<xsl:template match="rng:start">
 	&lt;ul&gt;
	<xsl:apply-templates select="child::*" />
 	&lt;/ul&gt;
</xsl:template>

<xsl:template match="/rng:grammar">
	<xsl:apply-templates select="rng:start"/>
</xsl:template>

</xsl:stylesheet>
