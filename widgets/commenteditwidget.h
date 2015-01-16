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
#ifndef COMMENTEDITWIDGET_H
#define COMMENTEDITWIDGET_H

#include <QDomComment>
#include <QWidget>

//----------------------------------------------------------------------

class QTableWidget;
class QPlainTextEdit;

//----------------------------------------------------------------------

class CommentEditWidget : public QWidget {
  Q_OBJECT
public:
  /*! Constructor.  QDomNode's are explicitly shared, all changes to "comment"
   * will propagate to the parent DOM document.
   * @param comment - the DOM comment represented by this widget
   * @param table - the parent table to add this widget to */
  explicit CommentEditWidget(QDomComment comment, QTableWidget *table);

signals:
  void contentsChanged();

private slots:
  void textChanged();

private:
  QDomComment m_comment;
  QTableWidget *m_table;
  QPlainTextEdit *m_textEdit;
};

#endif // COMMENTEDITWIDGET_H
