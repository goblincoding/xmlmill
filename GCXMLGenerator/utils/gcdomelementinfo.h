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

#ifndef GCDOMELEMENTINFO_H
#define GCDOMELEMENTINFO_H

#include <QStringList>
#include <QString>
#include <QDomElement>

/// QDomElement wrapper class.

/**
  A QDomElement wrapper that provides additional information in the XML Mill
  context regarding whether or not the element or any of its attributes should be excluded
  from the DOM document being built.
*/

class GCDomElementInfo
{
public:
  /*! Constructor. QDomElement's default shallow copying means we'll be working with the element directly. */
  explicit GCDomElementInfo( QDomElement element );

  /*! If "exclude" is true, the element is excluded from the active document. */
  void setExcludeElement( bool exclude );

  /*! Excludes "attribute" from the active document. */
  void excludeAttribute( const QString &attribute );

  /*! Includes "attribute" in the active document. */
  void includeAttribute( const QString &attribute );

  /*! Returns the wrapped element's name. */
  QString elementName() const;

  /*! Returns a list of all the attributes that should be included in the active document. */
  const QStringList &includedAttributes() const;

  /*! Returns "true" if the element should be excluded from the active document, "false" otherwise. */
  bool elementExcluded() const;

  /*! Provides a string representation of the element, its attributes and attribute values (including brackets
      and other XML characters). */
  QString toString() const;

  int index() const;

private:
  QDomElement m_element;
  bool m_elementExcluded;
  int  m_index;
  static int m_indexCount;

  QStringList m_includedAttributes;
};

#endif // GCDOMELEMENTINFO_H
