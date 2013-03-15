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
#include "gctreewidgetitem.h"
#include "gcglobalspace.h"

/*--------------------------------------------------------------------------------------*/

GCTreeWidgetItem::GCTreeWidgetItem( QDomElement element )
{
  init( element, -1 );
}

/*--------------------------------------------------------------------------------------*/

GCTreeWidgetItem::GCTreeWidgetItem( QDomElement element, int index )
{
  init( element, index );
}

/*--------------------------------------------------------------------------------------*/

void GCTreeWidgetItem::init( QDomElement element, int index )
{
  m_element = element;
  m_elementExcluded = false;
  m_index = index;
  m_verbose = GCGlobalSpace::showTreeItemsVerbose();

  QDomNamedNodeMap attributes = m_element.attributes();

  for( int i = 0; i < attributes.size(); ++i )
  {
    m_includedAttributes.append( attributes.item( i ).toAttr().name() );
  }

  setDisplayText();
}

/*--------------------------------------------------------------------------------------*/

void GCTreeWidgetItem::setDisplayText()
{
  if( m_verbose )
  {
    setText( 0, toString() );
  }
  else
  {
    setText( 0, m_element.tagName() );
  }
}

/*--------------------------------------------------------------------------------------*/

GCTreeWidgetItem *GCTreeWidgetItem::gcParent() const
{
  return dynamic_cast< GCTreeWidgetItem* >( parent() );
}

/*--------------------------------------------------------------------------------------*/

GCTreeWidgetItem *GCTreeWidgetItem::gcChild( int index ) const
{
  return dynamic_cast< GCTreeWidgetItem* >( child( index ) );
}

/*--------------------------------------------------------------------------------------*/

QDomElement GCTreeWidgetItem::element() const
{
  return m_element;
}

/*--------------------------------------------------------------------------------------*/

void GCTreeWidgetItem::setExcludeElement( bool exclude )
{
  if( gcParent() )
  {
    if( exclude )
    {
      gcParent()->element().removeChild( m_element );
    }
    else
    {
      gcParent()->element().appendChild( m_element );
    }
  }

  m_elementExcluded = exclude;
}

/*--------------------------------------------------------------------------------------*/

bool GCTreeWidgetItem::elementExcluded() const
{
  return m_elementExcluded;
}

/*--------------------------------------------------------------------------------------*/

void GCTreeWidgetItem::excludeAttribute( const QString &attribute )
{
  m_element.removeAttribute( attribute );
  m_includedAttributes.removeAll( attribute );
  m_includedAttributes.sort();
  setDisplayText();
}

/*--------------------------------------------------------------------------------------*/

void GCTreeWidgetItem::includeAttribute( const QString &attribute, const QString &value )
{
  m_element.setAttribute( attribute, value );
  m_includedAttributes.append( attribute );
  m_includedAttributes.removeDuplicates();
  m_includedAttributes.sort();
  setDisplayText();
}

/*--------------------------------------------------------------------------------------*/

bool GCTreeWidgetItem::attributeIncluded( const QString &attribute ) const
{
  return m_includedAttributes.contains( attribute );
}

/*--------------------------------------------------------------------------------------*/

void GCTreeWidgetItem::setIncrementAttribute( const QString &attribute, bool increment )
{
  if( increment )
  {
    m_incrementedAttributes.append( attribute );
  }
  else
  {
    m_incrementedAttributes.removeAll( attribute );
  }

  m_incrementedAttributes.sort();
}

/*--------------------------------------------------------------------------------------*/

bool GCTreeWidgetItem::incrementAttribute( const QString &attribute ) const
{
  return m_incrementedAttributes.contains( attribute );
}

/*--------------------------------------------------------------------------------------*/

void GCTreeWidgetItem::fixAttributeValues()
{
  m_fixedValues.clear();

  QDomNamedNodeMap attributes = m_element.attributes();

  for( int i = 0; i < attributes.size(); ++i )
  {
    QDomAttr attribute = attributes.item( i ).toAttr();
    m_fixedValues.insert( attribute.name(), attribute.value() );
  }
}

/*--------------------------------------------------------------------------------------*/

QString GCTreeWidgetItem::fixedValue( const QString &attribute ) const
{
  return m_fixedValues.value( attribute, QString() );
}

/*--------------------------------------------------------------------------------------*/

void GCTreeWidgetItem::revertToFixedValues()
{
  foreach( QString attribute, m_fixedValues.keys() )
  {
    m_element.setAttribute( attribute, m_fixedValues.value( attribute ) );
  }
}

/*--------------------------------------------------------------------------------------*/

QString GCTreeWidgetItem::toString() const
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

void GCTreeWidgetItem::setIndex( int index )
{
  m_index = index;
}

/*--------------------------------------------------------------------------------------*/

int GCTreeWidgetItem::index() const
{
  return m_index;
}

/*--------------------------------------------------------------------------------------*/

void GCTreeWidgetItem::rename( const QString &newName )
{
  m_element.setTagName( newName );
  setDisplayText();
}

/*--------------------------------------------------------------------------------------*/

QString GCTreeWidgetItem::name() const
{
  if( !m_element.isNull() )
  {
    return m_element.tagName();
  }

  return QString( "" );
}

/*--------------------------------------------------------------------------------------*/

void GCTreeWidgetItem::setVerbose( bool verbose )
{
  m_verbose = verbose;
  setDisplayText();
}

/*--------------------------------------------------------------------------------------*/
