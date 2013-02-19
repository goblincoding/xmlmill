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

#ifndef GCDOMTREEWIDGET_H
#define GCDOMTREEWIDGET_H

#include <QTreeWidget>

class QDomDocument;
class QDomElement;
class GCTreeWidgetItem;

/// Must be used in conjunction with GCTreeWidgetItem.

class GCDOMTreeWidget : public QTreeWidget
{
  Q_OBJECT
public:
  explicit GCDOMTreeWidget( QWidget *parent = 0 );
  ~GCDOMTreeWidget();

  void addElement( const QString &elementName, bool asSibling = false );
  void addComment( const QString &commentText );
  void clearAndReset();

  bool setContent( const QString & text, QString * errorMsg = 0, int * errorLine = 0, int * errorColumn = 0 );
  bool isDocumentCompatible() const;
  bool batchProcessDOMDocument() const;
  bool isEmpty() const;

  QDomElement root() const;
  QString toString() const;

  GCTreeWidgetItem *item( QDomElement element );
  const QList< GCTreeWidgetItem* > &items() const;

signals:
  void itemValueChanged( const QTreeWidgetItem *item );

private slots:
  /*! Connected to the "itemChanged( QTreeWidgetItem*, int )" signal.
      Called only when a user edits the name of an existing tree widget item
      (i.e. element). An element with the new name will be added to the DB
      (if it doesn't yet exist) with the same associated attributes and attribute
      values as the element name it is replacing (the "old" element will not be
      removed from the DB). All occurrences of the old name throughout the current
      DOM will be replaced with the new name and the tree widget will be updated
      accordingly. */
  void elementChanged( QTreeWidgetItem *item, int column );

private:
  QList< GCTreeWidgetItem* > m_items;
  QDomDocument *m_domDoc;
  bool          m_isEmpty;
  
};

#endif // GCDOMTREEWIDGET_H


