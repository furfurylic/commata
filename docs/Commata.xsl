<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://www.w3.org/1999/xhtml"
                version="1.0">
<xsl:output encoding="UTF-8" method="xml"
            doctype-public="-//W3C//DTD XHTML 1.1//EN"
            doctype-system="http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd"/>

<xsl:template match="/document">
  <html>
    <head>
      <title><xsl:apply-templates select="title/node()" mode="title"/></title>
      <style type="text/css"><xsl:text disable-output-escaping="yes">
        body { margin: 1em 5% 90em 5%; font-family: sans-serif }
        body * { line-height: 1.3em }
        h1 { font-size: 250%; margin-bottom: 0.4em; font-weight: normal }
        h2 { font-size: 220%; margin: 1.6em 0 0.8em 0; font-weight: normal }
        h3 { font-size: 180%; margin: 2em 0 1em 0; font-weight: normal }
        h2 + h3 { margin-top: 1em }
        h4 { font-size: 160%; margin: 2em 0 1em 0; font-weight: normal }
        h3 + h4 { margin-top: 1em }
        h5 { font-size: 140%; margin: 2.4em 0 1.2em 0; font-weight: normal }
        h4 + h5 { margin-top: 1.2em }
        h6 { font-size: 120%; margin: 2.4em 0 1.2em 0; font-weight: normal }
        h5 + h6 { margin-top: 1.2em }
        p, ul, li { margin: 0.5em 0 }

        #signature { text-align: right; margin-bottom: 2em }

        span.name { font-style: italic }
        code { color: #32308a; font-family: Courier, monospace; font-variant-ligatures: none }
        code a:link, pre a:visited, pre a:hover, pre a:active { color: inherit; text-decoration: none }
        code a:hover, pre a:active { text-decoration: underline }
        code span.not-a-code { font-style: italic }

        pre { line-height: 1.2em }
        pre.codeblock { border: solid 1px #dde; padding: 1em }
        pre span.comment { color: #915D2F }
        pre span.comment .note { font-style: italic; font-family: sans-serif }
        pre span.comment code { color: inherit; font-family: inherit }

        table { margin: 1em 0 1em 0; box-sizing: border-box; width: 100%; border: solid #dde; border-width: 2px 1px 1px 2px; caption-side: top }
        th, td { border: solid #dde; border-width: 0 1px 1px 0; vertical-align: top; text-align: left }
        th { border-bottom-style: double; border-bottom-width: 3px; padding: 0.4em }
        td { padding: 0.4em 0.4em 1em 0.4em }
        th p, td p { margin: 0 }
        th p + p, td p + p { margin-top: 0.4em }
        caption { text-align: left }

        div.code-item { margin-top: 1.2em }
        div.code-item > div.code-item-desc { margin-left: 3em }
        div.code-item pre { margin: 0.4em 0 }
        div.code-item p { margin-top: 0.4em; margin-bottom: 0.4em }
        div.code-item p span.code-item-heading { text-transform: capitalize; font-style: italic }

        p#toc-heading { font-size: 150%; margin-bottom: 0.8em }
        div#toc ul { list-style-type: none; margin: 0 }
        div#toc > ul { padding-left: 0 }
        div#toc li { margin: 0.2em 0 }
        div#toc a code  { color: inherit }

        @media print {
          body { margin: 0; font-family: sans-serif }
        }
      </xsl:text></style>
    </head>
    <body>
      <h1><xsl:apply-templates select="title/node()"/></h1>
      <p id="signature"><xsl:apply-templates select="signature"/></p>
      <div id="toc">
        <p id="toc-heading">Contents</p>
        <ul>
          <xsl:apply-templates select="section" mode="toc"/>
        </ul>
      </div>
      <xsl:apply-templates select="section"/>
    </body>
  </html>
</xsl:template>

<xsl:template match="*" mode="title">
  <xsl:value-of select="."/>
</xsl:template>

<xsl:template match="section" mode="toc">
  <li>
    <xsl:call-template name="toc-xref"/>
    <xsl:if test="section">
      <ul>
        <xsl:apply-templates select="section" mode="toc"/>
      </ul>
    </xsl:if>
  </li>
</xsl:template>

<xsl:template match="section">
  <xsl:variable name="heading-content"><xsl:apply-templates select="." mode="heading"/></xsl:variable>
  <xsl:variable name="depth"><xsl:apply-templates select="." mode="depth"/></xsl:variable>
  <xsl:choose>
    <xsl:when test="$depth = 1"><h2 id="{@id}"><xsl:copy-of select="$heading-content"/></h2></xsl:when>
    <xsl:when test="$depth = 2"><h3 id="{@id}"><xsl:copy-of select="$heading-content"/></h3></xsl:when>
    <xsl:when test="$depth = 3"><h4 id="{@id}"><xsl:copy-of select="$heading-content"/></h4></xsl:when>
    <xsl:when test="$depth = 4"><h5 id="{@id}"><xsl:copy-of select="$heading-content"/></h5></xsl:when>
    <xsl:when test="$depth = 5"><h6 id="{@id}"><xsl:copy-of select="$heading-content"/></h6></xsl:when>
    <xsl:otherwise><p id="{@id}"><xsl:copy-of select="$heading-content"/></p></xsl:otherwise>
  </xsl:choose>
  <xsl:apply-templates select="node()[local-name() != 'name']"/>
</xsl:template>

<xsl:template match="p">
  <p><xsl:apply-templates/></p>
</xsl:template>

<xsl:template match="ul">
  <ul><xsl:apply-templates/></ul>
</xsl:template>

<xsl:template match="ol">
  <ol><xsl:apply-templates/></ol>
</xsl:template>

<xsl:template match="li">
  <li><xsl:apply-templates/></li>
</xsl:template>

<xsl:template match="section" mode="outline">
  <xsl:number level="multiple" count="section" format="1.1.1.1.1"/>
</xsl:template>

<xsl:template match="table" mode="outline">
  <xsl:text>Table </xsl:text><xsl:number level="any" count="table" format="1"/>
</xsl:template>

<xsl:template match="section" mode="heading">
  <xsl:apply-templates select="." mode="outline"/>
  <xsl:text> </xsl:text>
  <xsl:apply-templates select="name"/>
</xsl:template>

<xsl:template match="table" mode="heading">
  <xsl:apply-templates select="." mode="outline"/><xsl:text>. </xsl:text>
  <xsl:apply-templates select="caption"/>
</xsl:template>

<xsl:template match="section" mode="depth">
  <xsl:variable name="num"><xsl:number level="multiple" count="section" format="1.1.1.1.1 "/></xsl:variable>
  <xsl:variable name="num2" select="string-length(normalize-space(translate($num, '0123456789', '          ')))"/>
  <xsl:choose>
    <xsl:when test="$num2 = 0">1</xsl:when>
    <xsl:when test="$num2 = 1">2</xsl:when>
    <xsl:otherwise><xsl:value-of select="($num2 - 1) div 2 + 2"/></xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="xref">
  <xsl:variable name="id" select="string(@id)"/>
  <a href="#{$id}"><xsl:apply-templates select="//*[@id = $id]" mode="outline"/></a>
</xsl:template>

<xsl:template name="toc-xref">
  <xsl:variable name="id" select="string(@id)"/>
  <a href="#{$id}"><xsl:apply-templates select="//*[@id = $id]" mode="heading"/></a>
</xsl:template>

<xsl:template match="c"><code><xsl:apply-templates/></code></xsl:template>
<xsl:template match="nc"><span class="not-a-code"><xsl:apply-templates/></span></xsl:template>

<xsl:template match="n"><span class="name"><xsl:apply-templates/></span></xsl:template>

<xsl:template match="code-item">
  <div class="code-item">
    <pre><code><xsl:apply-templates select="code" mode="code"/></code></pre>
    <div class="code-item-desc">
      <xsl:for-each select="preface">
        <p><xsl:apply-templates/></p>
      </xsl:for-each>
      <xsl:for-each select="effects|postcondition|requires|returns|remark|note|throws">
        <p><span class="code-item-heading"><xsl:value-of select="local-name(.)"/>: </span><xsl:apply-templates/></p>
      </xsl:for-each>
      <xsl:apply-templates select="table"/>
    </div>
  </div>
</xsl:template>

<xsl:template match="c" mode="code"><span class="comment"><xsl:apply-templates mode="code"/></span></xsl:template>

<xsl:template match="codeblock"><pre class="codeblock"><code><xsl:apply-templates mode="code"/></code></pre></xsl:template>
<xsl:template match="c/n" mode="code"><span class="note"><xsl:apply-templates/></span></xsl:template>
<xsl:template match="nc" mode="code"><span class="not-a-code"><xsl:apply-templates/></span></xsl:template>

<xsl:template match="text()" mode="code">
  <xsl:choose>
    <xsl:when test="not(preceding-sibling::node()) and following-sibling::node()">
      <xsl:call-template name="trim-left">
        <xsl:with-param name="s" select="string(.)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="preceding-sibling::node() and not(following-sibling::node())">
      <xsl:call-template name="trim-right">
        <xsl:with-param name="s" select="string(.)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:when test="not(preceding-sibling::node()) and not(following-sibling::node())">
      <xsl:call-template name="trim-left">
        <xsl:with-param name="s">
          <xsl:call-template name="trim-right">
            <xsl:with-param name="s" select="string(.)"/>
          </xsl:call-template>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise><xsl:value-of select="."/></xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="table">
  <table id="{@id}" cellspacing="0" cellpadding="0">
    <caption><xsl:apply-templates select="." mode="heading"/></caption>
    <xsl:variable name="total-width" select="sum(col/@width)"/>
    <xsl:for-each select="col">
      <col style="width: {@width div $total-width * 100}%"/>
    </xsl:for-each>
    <xsl:apply-templates select="tr"/>
  </table>
</xsl:template>

<xsl:template match="tr">
  <tr><xsl:apply-templates select="th|td"/></tr>
</xsl:template>

<xsl:template match="th">
  <th>
    <xsl:if test="@rowspan"><xsl:attribute name="rowspan"><xsl:value-of select="@rowspan"/></xsl:attribute></xsl:if>
    <xsl:apply-templates/>
  </th>
</xsl:template>

<xsl:template match="td">
  <td>
    <xsl:if test="@rowspan"><xsl:attribute name="rowspan"><xsl:value-of select="@rowspan"/></xsl:attribute></xsl:if>
    <xsl:apply-templates/>
  </td>
</xsl:template>

<xsl:template name="trim-left">
  <xsl:param name="s"/>
  <xsl:choose>
    <xsl:when test="starts-with($s, ' ') or starts-with($s, '&#xd;') or starts-with($s, '&#xa;') or starts-with($s, '&#x9;')">
      <xsl:call-template name="trim-left">
        <xsl:with-param name="s" select="substring($s, 2, string-length($s) - 1)"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise><xsl:value-of select="$s"/></xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="trim-right">
  <xsl:param name="s"/>
  <xsl:if test="string-length($s) &gt; 0">
    <xsl:variable name="e" select="substring($s, string-length($s), 1)"/>
    <xsl:choose>
      <xsl:when test="$e = ' ' or $e = '&#xd;' or $e = '&#xa;' or $e = '&#x9;'">
        <xsl:call-template name="trim-right">
          <xsl:with-param name="s" select="substring($s, 1, string-length($s) - 1)"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$s"/></xsl:otherwise>
    </xsl:choose>
  </xsl:if>
</xsl:template>

</xsl:stylesheet>
