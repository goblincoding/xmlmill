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

#ifndef GCTREEWIDGETITEM_H
#define GCTREEWIDGETITEM_H

#include <QTreeWidgetItem>
#include <QStringList>
#include <QString>
#include <QDomElement>

/// Used in GCDomTreeWidget, each GCTreeWidgetItem can be associated with a QDomElement.

/*!
    Can be associated with a QDomElement in order to provide additional information in the XML Mill
    context regarding whether or not the element or any of its attributes should be excluded
    from the DOM document being built.  This item DOES NOT OWN the QDomElement (hence all the non
    const return values...QDomElement's default copy constructor is shallow).
*/

class GCTreeWidgetItem : public QTreeWidgetItem
{
public:
  /*! Constructor. */
  explicit GCTreeWidgetItem( const QString &elementName );

  /*! Constructor. Associates "element" with this item. */
  explicit GCTreeWidgetItem( const QString &elementName, QDomElement element );

  /*! Constructor. Associates "element" with this item and assigns it an "index"
      which is used to determine the underlying DOM element's relative position
      within the DOM (roughly corresponding to "line numbers"). */
  explicit GCTreeWidgetItem( const QString &elementName, QDomElement element, int index );

  /*! Returns the parent item as a GCTreeWidgetItem. */
  GCTreeWidgetItem *gcParent() const;

  /*! Returns the child item at "index" as a GCTreeWidgetItem. */
  GCTreeWidgetItem *gcChild( int index ) const;

  /*! Returns the associated element via QDomElement's default shallow copy constructor. */
  QDomElement element() const;

  /*! If "exclude" is true, the element is excluded from the active document. */
  void setExcludeElement( bool exclude );

  /*! Excludes "attribute" from the active document. */
  void excludeAttribute( const QString &attribute );

  /*! Includes "attribute" with "value" in the active document. */
  void includeAttribute( const QString &attribute, const QString &value );

  /*! Returns a list of all the attributes that should be included in the active document. */
  const QStringList &includedAttributes() const;

  /*! Returns "true" if the element should be excluded from the active document, "false" otherwise. */
  bool elementExcluded() const;

  /*! Provides a string representation of the element, its attributes and attribute values (including brackets
      and other XML characters). */
  QString toString() const;

  /*! Returns the index associated with this element.  Indices in this context are rough indications
      of an element's relative position within the DOM document (approximating "line numbers"). */
  int index() const;

private:
  void init( const QString &elementName, QDomElement element, int index );

  QDomElement m_element;
  bool m_elementExcluded;
  int  m_index;

  QStringList m_includedAttributes;

};

#endif // GCTREEWIDGETITEM_H
