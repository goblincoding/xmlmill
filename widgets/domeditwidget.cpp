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
#include "domeditwidget.h"
#include "domnodeedit.h"
#include "model/domitem.h"

#include <QVBoxLayout>
#include <QModelIndex>

#include <assert.h>

//----------------------------------------------------------------------

DomEditWidget::DomEditWidget(QWidget *parent)
    : QWidget(parent), m_nodeEdits(), m_layout(nullptr) {
}

//----------------------------------------------------------------------

void DomEditWidget::indexSelected(const QModelIndex &index) {
  assert(index.isValid());

  if (index.isValid()) {
    qDeleteAll(m_nodeEdits);
    m_nodeEdits.clear();

    delete m_layout;
    m_layout = new QVBoxLayout(this);

    DomItem *item = static_cast<DomItem *>(index.internalPointer());
    addNodeEdit(item);

    for (DomItem *child : item->childItems()) {
      addNodeEdit(child);
    }

    this->setLayout(m_layout);
  }
}

//----------------------------------------------------------------------

void DomEditWidget::addNodeEdit(DomItem *item) {
  assert(!item->node().isNull());

  if (!item->node().isNull() && item->node().isElement()) {
    DomNodeEdit *edit = new DomNodeEdit(item->node().toElement(), this);

    if (edit->hasContent()) {
      m_layout->addWidget(edit);
    }

    /* Add it to the list regardless of whether it has content so we can clean
     * it up later. */
    m_nodeEdits.append(edit);
  }
}

//----------------------------------------------------------------------
