/* Copyright (c) 2012 by William Hallatt.
 *
 * This file forms part of "XML Mill".
 *
 * The official website for this project is <http://www.goblincoding.com> and,
 * although not compulsory, it would be appreciated if all works of whatever
 * nature using this source code (in whole or in part) include a reference to
 * this site.
 *
 * Should you wish to contact me for whatever reason, please do so via:
 *
 *                 <http://www.goblincoding.com/contact>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#include "gcdomelementinfo.h"


/*--------------------------------------------------------------------------------------*/

GCDomElementInfo::GCDomElementInfo( QDomElement element ) :
  m_element           ( element ),
  m_elementExcluded   ( false ),
  m_includedAttributes()
{
  QDomNamedNodeMap attributes = m_element.attributes();

  for( int i = 0; i < attributes.size(); ++i )
  {
    m_includedAttributes.append( attributes.item( i ).toAttr().name() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCDomElementInfo::setExcludeElement( bool exclude )
{
  m_elementExcluded = exclude;
}

/*--------------------------------------------------------------------------------------*/

void GCDomElementInfo::excludeAttribute( const QString &attribute )
{
  m_includedAttributes.removeAll( attribute );
}

/*--------------------------------------------------------------------------------------*/

void GCDomElementInfo::includeAttribute( const QString &attribute )
{
  m_includedAttributes.append( attribute );
}

/*--------------------------------------------------------------------------------------*/

QString GCDomElementInfo::elementName() const
{
  return m_element.tagName();
}

/*--------------------------------------------------------------------------------------*/

const QStringList &GCDomElementInfo::includedAttributes() const
{
  return m_includedAttributes;
}

/*--------------------------------------------------------------------------------------*/

bool GCDomElementInfo::elementExcluded() const
{
  return m_elementExcluded;
}

/*--------------------------------------------------------------------------------------*/

QString GCDomElementInfo::toString() const
{
  QString text( "<" );
  text += m_element.tagName();

  QDomNamedNodeMap attributes = m_element.attributes();

  /* For elements with no attributes (e.g. <element/>). */
  if( attributes.isEmpty() &&
      m_element.childNodes().isEmpty() )
  {
    text += "/>";
    return text;
  }

  if( !attributes.isEmpty() )
  {
    for( int i = 0; i < attributes.size(); ++i )
    {
      text += " ";

      QString attribute = attributes.item( i ).toAttr().name();
      text += attribute;
      text += "=\"";

      QString attributeValue = attributes.item( i ).toAttr().value();
      text += attributeValue;
      text += "\"";
    }

    /* For elements without children but with attributes. */
    if( m_element.firstChild().isNull() )
    {
      text += "/>";
    }
    else
    {
      /* For elements with children and attributes. */
      text += ">";
    }
  }
  else
  {
    text += ">";
  }

  return text;
}

/*--------------------------------------------------------------------------------------*/
