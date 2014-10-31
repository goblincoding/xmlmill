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
#include "domdelegate.h"

#include <QApplication>
#include <QPainter>
#include <QPlainTextEdit>

#include "xmlsyntaxhighlighter.h"

/* Information on paint and sizeHint methods originally obtained from
 * http://stackoverflow.com/questions/1956542/how-to-make-item-view-render-rich-html-text-in-qt
 */

//----------------------------------------------------------------------

DomDelegate::DomDelegate(QObject *parent)
    : QStyledItemDelegate(parent), m_textDocument() {
  /* Create the document and initialise the font, setting the document margin to
   * 0 allows us to change the font (e.g. via style sheets) without affecting
   * the layout. */
  QTextOption textOption;
  textOption.setWrapMode(QTextOption::WordWrap);
  m_textDocument.setDefaultTextOption(textOption);

  /* Everything happens automagically after we add the syntax highlighter to the
   * document. */
  XmlSyntaxHighlighter* highLighter = new XmlSyntaxHighlighter(&m_textDocument);
  Q_UNUSED(highLighter);
}

//----------------------------------------------------------------------

void DomDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const {
  /* Create a QStyleOptionViewItem that we can manipulate locally. */
  QStyleOptionViewItemV4 optionV4 = option;
  initStyleOption(&optionV4, index);

  editTextDocument(optionV4);

  /* Get the style associated with the option widget (if we have one). */
  QStyle *style =
      optionV4.widget ? optionV4.widget->style() : QApplication::style();

  /* Draw the item view item without text (required for highlighting
   * etc to work as expected). */
  optionV4.text = QString();
  style->drawControl(QStyle::CE_ItemViewItem, &optionV4, painter);

  /* Determine the correct text area to paint. */
  QRect textRect =
      style->subElementRect(QStyle::SE_ItemViewItemText, &optionV4);

  painter->save();
  painter->translate(textRect.topLeft());
  painter->setClipRect(textRect.translated(-textRect.topLeft()));

  /* Adjust for highlighted text. */
  QAbstractTextDocumentLayout::PaintContext ctx;

  if (optionV4.state & QStyle::State_Selected) {
    ctx.palette.setColor(
        QPalette::Text,
        optionV4.palette.color(QPalette::Active, QPalette::HighlightedText));
  } else {
    ctx.palette.setColor(QPalette::Text, optionV4.palette.color(
                                             QPalette::Active, QPalette::Text));
  }

  m_textDocument.documentLayout()->draw(painter, ctx);
  painter->restore();
}

//----------------------------------------------------------------------

QSize DomDelegate::sizeHint(const QStyleOptionViewItem &option,
                            const QModelIndex &index) const {
  QStyleOptionViewItemV4 optionV4 = option;
  initStyleOption(&optionV4, index);
  editTextDocument(optionV4);
  return QSize(m_textDocument.idealWidth(), m_textDocument.size().height());
}

//----------------------------------------------------------------------

QWidget *DomDelegate::createEditor(QWidget *parent,
                                   const QStyleOptionViewItem & /*option*/,
                                   const QModelIndex & /*index*/) const {
  QPlainTextEdit *textEdit = new QPlainTextEdit(parent);
  return textEdit;
}

//----------------------------------------------------------------------

void DomDelegate::setEditorData(QWidget *editor,
                                const QModelIndex &index) const {
  QString value = index.model()->data(index, Qt::DisplayRole).toString();
  QPlainTextEdit *textEdit = dynamic_cast<QPlainTextEdit *>(editor);

  if (textEdit) {
    textEdit->setPlainText(value);
  }
}

//----------------------------------------------------------------------

void DomDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                               const QModelIndex &index) const {
  QPlainTextEdit *textEdit = dynamic_cast<QPlainTextEdit *>(editor);

  if (textEdit) {
    QString value = textEdit->toPlainText();
    model->setData(index, value, Qt::DisplayRole);
  }
}

//----------------------------------------------------------------------

void DomDelegate::updateEditorGeometry(QWidget *editor,
                                       const QStyleOptionViewItem &option,
                                       const QModelIndex & /*index*/) const {
  editor->setGeometry(option.rect);
}

//----------------------------------------------------------------------

void DomDelegate::editTextDocument(QStyleOptionViewItemV4 &option) const {
  m_textDocument.setDocumentMargin(0);
  m_textDocument.setDefaultFont(option.font);
  m_textDocument.setPlainText(option.text);
  m_textDocument.setTextWidth(option.widget->rect().width());
}

//----------------------------------------------------------------------
