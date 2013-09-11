/* Copyright (c) 2012 - 2013 by William Hallatt.
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

/// Used by GCDomTreeWidget, each GCTreeWidgetItem can be associated with a QDomElement.

/**
    Can be associated with a QDomElement in order to provide additional information in the XML Mill
    context regarding whether or not the element or any of its attributes should be excluded
    from the DOM document being built.  This item DOES NOT OWN the QDomElement (hence all the non
    const return values...QDomElement's default copy constructor is shallow).

    Some specialisation specific to GCAddSnippetsForm was added to this class (all functions related to
    incrementation and fixing of attribute values).  The reason this functionality was added here is
    due to complications derived from the default shallow copy constructors of QDomAttr (I originally
    tried to use maps confined to GCAddSnippetForm objects, but to no avail).
*/

class GCTreeWidgetItem : public QTreeWidgetItem
{
public:
  /*! Constructor. Associates "element" with this item. */
  explicit GCTreeWidgetItem( QDomElement element );

  /*! Constructor. Associates "element" with this item and assigns it an "index"
      which is used to determine the underlying DOM element's relative position
      within the DOM (roughly corresponding to "line numbers"). */
  explicit GCTreeWidgetItem( QDomElement element, int index );

  /*! Returns this item's parent item as a GCTreeWidgetItem. */
  GCTreeWidgetItem* gcParent() const;

  /*! Returns this item's child item at "index" as a GCTreeWidgetItem. */
  GCTreeWidgetItem* gcChild( int index ) const;

  /*! Returns the associated element via QDomElement's default shallow copy constructor. */
  QDomElement element() const;

  /*! Sets the "exclude" flag used to determine whether or not the element must be included in GCDomTreeWidget's
      DOM document (elements are included by default).
      \sa elementExcluded */
  void setExcludeElement( bool exclude );

  /*! Returns "true" if the element should be excluded from the active document (elements are included
      by default).
      \sa setExcludeElement */
  bool elementExcluded() const;

  /*! Removes "attribute" from the underlying DOM element.
      \sa includeAttribute
      \sa attributeIncluded */
  void excludeAttribute( const QString& attribute );

  /*! Includes "attribute" with "value" in the underlying DOM element.
      \sa excludeAttribute
      \sa attributeIncluded */
  void includeAttribute( const QString& attribute, const QString& value );

  /*! Returns true if the underlying element contains "attribute".
      \sa excludeAttribute
      \sa includeAttribute */
  bool attributeIncluded( const QString& attribute ) const;

  /*! Adds "attribute" to a list of attributes whose values must be incremented when multiple snippets
      are added to the active DOM.  The reason this functionality was added here is due to the complications
      inherent in the default shallow copy constructors of QDomAttr (I originally tried to use maps confined
      to GCAddSnippetForm objects, but to no avail).
      \sa incrementAttribute
      \sa fixAttributeValues
      \sa fixedValue
      \sa revertToFixedValues */
  void setIncrementAttribute( const QString& attribute, bool increment );

  /*! Returns true if "attribute" must be incremented automatically when multiple snippets
      are added to the active DOM.
      \sa setIncrementAttribute
      \sa fixAttributeValues
      \sa fixedValue
      \sa revertToFixedValues */
  bool incrementAttribute( const QString& attribute ) const;

  /*! Takes a snapshot of the current attribute values so that element attributes may be incremented
      automatically with reference to (and without losing) the original values.
      \sa incrementAttribute
      \sa setIncrementAttribute
      \sa fixedValue
      \sa revertToFixedValues */
  void fixAttributeValues();

  /*! Returns the fixed value saved against "attribute".
      \sa incrementAttribute
      \sa setIncrementAttribute
      \sa fixAttributeValues
      \sa revertToFixedValues */
  QString fixedValue( const QString& attribute ) const;

  /*! Reverts to the attribute values set with the "fixedAttributeValues" call.
      \sa setIncrementAttribute
      \sa incrementAttribute
      \sa fixAttributeValues
      \sa fixedValue */
  void revertToFixedValues();

  /*! Provides a string representation of the element, its attributes and attribute values (including brackets
      and other XML characters). */
  QString toString() const;

  /*! Sets the item's index to "index".
      \sa index */
  void setIndex( int index );

  /*! Returns the index associated with this element. Indices in this context are rough indications
      of an element's relative position within the DOM document (approximating "line numbers").
      \sa setIndex */
  int index() const;

  /*! Renames the element to "newName". */
  void rename( const QString& newName );

  /*! Returns the element name. */
  QString name() const;

  /*! Sets the item's element display as "verbose". When "verbose", the entire node is displayed (element
      attributes and values), otherwise only the element name is displayed.
      \sa setDisplayText */
  void setVerbose( bool verbose );

  /*! Inserts "item" at "index" and ensures that the corresponding DOM element is also
      inserted in the correct position (relative to the item's siblings). */
  void insertGcChild( int index, GCTreeWidgetItem* item );

private:
  /*! Initialise the item. */
  void init( QDomElement element, int index );

  /*! Sets the item's text depending on the "verbose" flag.
      \sa setVerbose */
  void setDisplayText();

  QDomElement m_element;
  bool m_elementExcluded;
  bool m_verbose;
  int m_index;

  QStringList m_includedAttributes;
  QStringList m_incrementedAttributes;
  QHash< QString/*attr*/, QString /*val*/ > m_fixedValues;
};

#endif // GCTREEWIDGETITEM_H