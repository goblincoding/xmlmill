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
#ifndef DOMDELEGATE_H
#define DOMDELEGATE_H

#include <QStyledItemDelegate>
#include <QTextDocument>

//----------------------------------------------------------------------

class DomDelegate : public QStyledItemDelegate {
  Q_OBJECT
public:
  explicit DomDelegate(QObject *parent = 0);

  // QAbstractItemDelegate interface
public:
  virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
                     const QModelIndex &index) const;
//  virtual QSize sizeHint(const QStyleOptionViewItem &option,
//                         const QModelIndex &index) const;
  virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
                                const QModelIndex &) const;
  virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
  virtual void setModelData(QWidget *editor, QAbstractItemModel *model,
                            const QModelIndex &index) const;
  virtual void updateEditorGeometry(QWidget *editor,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &) const;

private:
  void editTextDocument(QStyleOptionViewItemV4 &option) const;

private:
  mutable QTextDocument m_textDocument;
};

#endif // DOMDELEGATE_H
