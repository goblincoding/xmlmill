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

/*----------------------------------------------------------------------------------------

  This class wraps a DOM element and provides additional information in the XML Mill
  context regarding whether or not the element or any of its attributes should be excluded
  from the document.

----------------------------------------------------------------------------------------*/

class GCDomElementInfo
{
public:
  /* QDomElement's default shallow copying means we'll be working with the element directly. */
  explicit GCDomElementInfo( QDomElement element );

  void setExcludeElement( bool exclude );
  void excludeAttribute ( const QString &attribute );
  void includeAttribute ( const QString &attribute );

  QString elementName() const;
  const QStringList &includedAttributes() const;
  bool elementExcluded() const;

  /* Provides a textual representation of the element and its attributes (including brackets
    and other XML characters). */
  QString toString() const;

private:
  QDomElement m_element;
  bool m_elementExcluded;

  QStringList m_includedAttributes;

};

#endif // GCDOMELEMENTINFO_H
