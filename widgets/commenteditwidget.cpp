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

#include "commenteditwidget.h"
#include "elementeditwidget.h"

#include <QPlainTextEdit>
#include <QTableWidget>
#include <assert.h>

//----------------------------------------------------------------------

CommentEditWidget::CommentEditWidget(QDomComment comment, QTableWidget *table)
    : QWidget(nullptr), m_comment(comment), m_table(table),
      m_textEdit(nullptr) {
  assert(!m_comment.isNull());
  assert(m_table);

  if (!m_comment.isNull()) {
    const int row = m_table->rowCount();
    m_table->setRowCount(row + 1);

    m_textEdit = new QPlainTextEdit();
    m_textEdit->setPlainText(m_comment.nodeValue());
    connect(m_textEdit, SIGNAL(textChanged()), this, SLOT(textChanged()));

    using Columns = ElementEditWidget::Columns;
    // Takes ownership.
    m_table->setCellWidget(
        row, ElementEditWidget::intFromEnum(Columns::Attribute), m_textEdit);
    m_table->setSpan(row, ElementEditWidget::intFromEnum(Columns::Attribute), 1,
                     ElementEditWidget::intFromEnum(Columns::Count));
  }
}

//----------------------------------------------------------------------

void CommentEditWidget::textChanged() {
  m_comment.setNodeValue(m_textEdit->toPlainText());
  emit contentsChanged();
}

//----------------------------------------------------------------------
