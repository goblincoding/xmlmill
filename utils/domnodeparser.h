/* Copyright (c) 2012 - 2015 by William Hallatt.
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
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */
#ifndef DOMNODEPARSER_H
#define DOMNODEPARSER_H

#include <QDomNode>

/*! A convenience class for converting a QDomNode to its textual, QString
 * representation or a QString to a corresponding QDomNode. */
class DomNodeParser {
public:
  /*! Returns the XML string representation (including special characters) of
   * QDomNode. */
  QString toString(const QDomNode &node) const;

  /*! Returns a QDomNode corresponding to "xml" by means of
   * QDomNode::cloneNode() */
  QDomNode toDomNode(const QString &xml) const;
};

#endif // DOMNODEPARSER_H
