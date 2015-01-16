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
#include "domnodeparser.h"

#include <assert.h>

//----------------------------------------------------------------------

QString DomNodeParser::toString(const QDomNode &node) const {
  if (!node.isNull()) {
    if (node.isElement()) {
      return node.nodeName();
    } else if (node.isComment()) {
      return QString("<<-- %1 -->").arg(node.nodeValue());
    } else if (node.isProcessingInstruction()) {
      return "FIND THIS STRING AND FIX IT!";
    } else if(node.isDocument()) {
      return QString();
    }

    assert(false && "Missing a node type that we should have catered for.");
    return QString("");
  }

  return QString();
}

//----------------------------------------------------------------------

QDomNode DomNodeParser::toDomNode(const QString &xml) const {
  QDomDocument doc;
  QString xmlErr("");
  int line(-1);
  int col(-1);

  if (doc.setContent(xml, &xmlErr, &line, &col)) {
    return doc.documentElement().cloneNode();
  }

  assert(false && "DomNodeParser::elementNode XML broken");
  return QDomNode();
}

//----------------------------------------------------------------------
