<?xml version="1.0" encoding="ISO-8859-1"?>


<!-- 

     met.xsl

     by Joern Thyssen <jthyssen@dk.ibm.com>, 2002

     This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.

     $Id: met.xsl,v 1.6 2013/08/21 03:45:23 mdpetch Exp $

 -->     

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output encoding="ISO-8859-1" 
              indent="yes" 
              method="html" 
              omit-xml-declaration="yes" 
              doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN" 
              doctype-system="http://www.w3.org/TR/html4/transitional.dtd"
   />


  <xsl:template match="/">
    <html>
      <xsl:apply-templates select=".//info" />

      <body>         
 
      <h1>Match equity table: <xsl:value-of select=".//info/name"/></h1>

      <h2>Description</h2>

      <p>
      <xsl:apply-templates select=".//info/description" />
      </p>

      <h2>Pre-Crawford table</h2>
    
      <xsl:apply-templates select=".//pre-crawford-table" />

      <h2>Post-Crawford table</h2>
    
      <xsl:apply-templates select=".//post-crawford-table" />


      </body>
    </html>
  </xsl:template>


  <xsl:template match="//info">
    <head>
       <xsl:apply-templates select="name" />
    </head>
  </xsl:template>
  
  <xsl:template match="//info/name">
    <title>
       Match equity table: <xsl:apply-templates />
    </title>
  </xsl:template>


  <xsl:template match="//info/description">
    <xsl:apply-templates />
  </xsl:template>

  <xsl:template match="parameters">

     <xsl:for-each select="parameter">
     <tr>
       <td><xsl:value-of select="@name" /></td>
       <td><xsl:apply-templates /></td>
     </tr>
     </xsl:for-each>
  </xsl:template>


  <xsl:template name="me">

  <!-- convert to percentages -->

  <xsl:choose>
    <xsl:when test="current() >= 0.1">
      <xsl:value-of select="format-number(100.0 * current(), '#0.0')" />
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="format-number(100.0 * current(), '#0.00')" />
    </xsl:otherwise>
  </xsl:choose>

  </xsl:template>


  <xsl:template name="table">
  <xsl:choose>
     <xsl:when test="@type = 'explicit'">
     <table border="1" cellpadding="2">
        <!-- table header -->

        <tr>
        <th></th>
        <xsl:for-each select="row[position()=1]/me">
        <th><xsl:number value="position()" />-away</th>
        </xsl:for-each>
        </tr>

        <xsl:for-each select="row">
        
           <tr>
              <td align="right"><strong><xsl:number value="position()" />-away</strong></td>
           <xsl:for-each select="me">
           <td align="right"><xsl:call-template name="me" /></td>
           </xsl:for-each>
           </tr>
        </xsl:for-each>

     </table>
     </xsl:when>
  <xsl:otherwise>
     <table border="1" cellpadding="2">
        <tr><th>Parameter</th><th>Value</th></tr>
        <tr><td>Name of generator</td><td><xsl:value-of select="@type" /></td></tr>
        <xsl:apply-templates />
     </table>
  </xsl:otherwise>
  </xsl:choose>
  </xsl:template>

  <xsl:template match="//pre-crawford-table">
  <xsl:call-template name="table" />
  </xsl:template>

  <xsl:template match="//post-crawford-table">
  <xsl:call-template name="table" />
  </xsl:template>

</xsl:stylesheet>
