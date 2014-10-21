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

#include <QModelIndex>
#include <QHeaderView>
#include <QPushButton>

#include <assert.h>

//----------------------------------------------------------------------

/* The number of children we add per increment for indices with large
 * amounts of child items. */
const int c_childCountIncrement = 10;

using Columns = DomNodeEdit::Columns;

//----------------------------------------------------------------------

DomEditWidget::DomEditWidget(QWidget *parent)
    : QTableWidget(parent), m_nodeEdits(), m_currentItem(nullptr),
      m_childrenProcessed(0) {
  setupTable();
}

//----------------------------------------------------------------------

void DomEditWidget::treeIndexSelected(const QModelIndex &index) {
  resetState();

  if (index.isValid()) {
    DomItem *item = static_cast<DomItem *>(index.internalPointer());

    if (item != m_currentItem) {
      m_currentItem = item;
      processItem(m_currentItem);
      processChildItems();
      resizeColumnToContents(DomNodeEdit::intFromEnum(Columns::Attribute));
    }
  }
}

//----------------------------------------------------------------------

void DomEditWidget::setupTable() {
  QStringList header;
  header << "Attribute:"
         << "Value:";
  setColumnCount(DomNodeEdit::intFromEnum(Columns::Count));
  setHorizontalHeaderLabels(header);
  horizontalHeader()->setStretchLastSection(true);
}

//----------------------------------------------------------------------

void DomEditWidget::resetState() {
  clearContents();
  setRowCount(0);
  qDeleteAll(m_nodeEdits);
  m_nodeEdits.clear();
  m_currentItem = nullptr;
  m_childrenProcessed = 0;
}

//----------------------------------------------------------------------

void DomEditWidget::createAddMoreButton() {
  setRowCount(rowCount() + 1);
  const int finalRow = rowCount() - 1;
  const int column = DomNodeEdit::intFromEnum(Columns::Attribute);

  QPushButton *button = new QPushButton("Load More", this);
  connect(button, SIGNAL(clicked()), this, SLOT(processChildItems()));

  setCellWidget(finalRow, column, button);
  setSpan(finalRow, column, 1, DomNodeEdit::intFromEnum(Columns::Count));
  updateGeometries();
}

//----------------------------------------------------------------------

void DomEditWidget::removeAddMoreButton() {
  const int finalRow = rowCount() - 1;
  const int column = DomNodeEdit::intFromEnum(Columns::Attribute);
  QPushButton *button =
      dynamic_cast<QPushButton *>(cellWidget(finalRow, column));

  if (button) {
    removeRow(finalRow);
    delete button;
  }
}

//----------------------------------------------------------------------

void DomEditWidget::processItem(DomItem *item) {
  assert(item && !m_currentItem->node().isNull());

  if (item) {
    QDomElement element = item->node().toElement();

    if (!element.isNull()) {
      DomNodeEdit *edit = new DomNodeEdit(element, this);
      m_nodeEdits.append(edit);
    }
  }
}

//----------------------------------------------------------------------

void DomEditWidget::processChildItems() {
  assert(m_currentItem);

  if (m_currentItem) {
    removeAddMoreButton();

    auto children = m_currentItem->childItems();
    const int incrementCount = m_childrenProcessed + c_childCountIncrement;
    const bool incrementalUpdate = incrementCount < children.size();
    const int count = incrementalUpdate ? incrementCount : children.size();

    for (int i = m_childrenProcessed; i < count; ++i) {
      auto child = children.at(i);
      processItem(child);
      ++m_childrenProcessed;
    }

    if (incrementalUpdate) {
      createAddMoreButton();
    }
  }
}

//----------------------------------------------------------------------
