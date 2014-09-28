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

//----------------------------------------------------------------------

QString DomNodeParser::toString(const QDomNode &node) const {
  if (node.isElement()) {
    return elementString(node);
  } else if (node.isComment()) {
    return commentString(node);
  } else if (node.isProcessingInstruction()) {
    return "FIND THIS STRING AND FIX IT!";
  }

  return node.nodeValue();
}

//----------------------------------------------------------------------

QString DomNodeParser::elementString(const QDomNode &node) const {
  QString text("<");
  text += node.nodeName();

  QDomNamedNodeMap attributeMap = node.attributes();

  /* For elements with no attributes (e.g. <element/>). */
  if (attributeMap.isEmpty() && node.childNodes().isEmpty()) {
    text += "/>";
    return text;
  }

  if (!attributeMap.isEmpty()) {
    for (int i = 0; i < attributeMap.size(); ++i) {
      QDomNode attribute = attributeMap.item(i);
      text += " ";
      text += attribute.nodeName();
      text += "=\"";
      text += attribute.nodeValue();
      text += "\"";
    }

    /* For elements without children but with attributes. */
    if (node.firstChild().isNull()) {
      text += "/>";
    } else {
      /* For elements with children and attributes. */
      text += ">";
    }
  } else {
    text += ">";
  }

  return text;
}

//----------------------------------------------------------------------

QString DomNodeParser::commentString(const QDomNode &node) const {
  QString text("<!-- ");
  text += node.nodeValue();
  text += " -->";
  return text;
}

//----------------------------------------------------------------------
