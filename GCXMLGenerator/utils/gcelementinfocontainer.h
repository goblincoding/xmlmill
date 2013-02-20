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

#ifndef GCELEMENTINFOCONTAINER_H
#define GCELEMENTINFOCONTAINER_H

#include <QHash>

class QTreeWidgetItem;
class QDomElement;
class GCElementInfo;

/// A container class in charge of mapping QTreeWidgetItems to their corresponding QDomElements.

/*! This class is responsible for managing relationships between QTreeWidgetItems, QDomElements and
    GCElementInfo objects.  This class does not own any objects other than the element info wrappers
    (hence the non-const return values of all the getters), but simply provides convenient mappings and
    easy access to the various objects. */
class GCElementInfoContainer
{
public:
  explicit GCElementInfoContainer();
  ~GCElementInfoContainer();

  QTreeWidgetItem *treeWidgetItem( QDomElement element ) const;
  QDomElement element( QTreeWidgetItem *item ) const;
  GCElementInfo *elementInfo( QTreeWidgetItem *item ) const;

  QList< QDomElement > elements();

  void insert( QTreeWidgetItem *item, QDomElement element );
  void remove( QTreeWidgetItem *item );

  bool contains( QTreeWidgetItem* item );
  bool isEmpty();

  /*! Cleans up and clears the maps. */
  void clear();

private:
  QHash< QTreeWidgetItem*, GCElementInfo* > m_itemInfo;
  QHash< QTreeWidgetItem*, QDomElement >    m_itemElement;

};

#endif // GCELEMENTINFOCONTAINER_H
