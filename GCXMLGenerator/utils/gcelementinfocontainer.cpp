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

#include "gcelementinfocontainer.h"
#include "gcdomelementinfo.h"

#include <QDomElement>
#include <QTreeWidgetItem>

/*--------------------------------------------------------------------------------------*/

GCElementInfoContainer::GCElementInfoContainer() :
  m_itemInfo   (),
  m_itemElement()
{
}

/*--------------------------------------------------------------------------------------*/

GCElementInfoContainer::~GCElementInfoContainer()
{
  clear();
}

/*--------------------------------------------------------------------------------------*/

QTreeWidgetItem *GCElementInfoContainer::treeWidgetItem( QDomElement element ) const
{
  return m_itemElement.key( element );
}

/*--------------------------------------------------------------------------------------*/

QDomElement GCElementInfoContainer::element( QTreeWidgetItem *item ) const
{
  return m_itemElement.value( item );
}

/*--------------------------------------------------------------------------------------*/

GCDomElementInfo *GCElementInfoContainer::elementInfo( QTreeWidgetItem *item ) const
{
  return m_itemInfo.value( item );
}

/*--------------------------------------------------------------------------------------*/

QList< QDomElement > GCElementInfoContainer::elements()
{
  return m_itemElement.values();
}

/*--------------------------------------------------------------------------------------*/

void GCElementInfoContainer::insert( QTreeWidgetItem *item, QDomElement element )
{
  m_itemElement.insert( item, element );
  m_itemInfo.insert( item, new GCDomElementInfo( element ) );
}

/*--------------------------------------------------------------------------------------*/

void GCElementInfoContainer::remove( QTreeWidgetItem *item )
{
  m_itemElement.remove( item );

  GCDomElementInfo *info = m_itemInfo.value( item );
  delete info;
  info = NULL;
  m_itemInfo.remove( item );
}

/*--------------------------------------------------------------------------------------*/

bool GCElementInfoContainer::contains( QTreeWidgetItem *item )
{
  return m_itemElement.contains( item );
}

/*--------------------------------------------------------------------------------------*/

bool GCElementInfoContainer::isEmpty()
{
  return m_itemElement.isEmpty();
}

/*--------------------------------------------------------------------------------------*/

void GCElementInfoContainer::clear()
{
  foreach( GCDomElementInfo *info, m_itemInfo.values() )
  {
    delete info;
    info = NULL;
  }

  m_itemInfo.clear();
  m_itemElement.clear();
}

/*--------------------------------------------------------------------------------------*/
