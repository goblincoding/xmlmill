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

#ifndef GCSNIPPETSFORM_H
#define GCSNIPPETSFORM_H

#include <QDialog>
#include <QHash>
#include <QDomDocument>

namespace Ui
{
  class GCSnippetsForm;
}

class GCDomElementInfo;
class QTreeWidgetItem;
class QTableWidgetItem;
class QSignalMapper;
class QCheckBox;

/*------------------------------------------------------------------------------------------

  This form allows the user to add multiple XML snippets of the same structure with whichever
  default values the user specifies.  It furthermore allows for the option to increment the
  default values for each snippet (i.e. if the user specifies "1" as an attribute value with
  the option to increment, then the next snippet generated will have "2" as the value for the
  same attribute and so on and so forth.  Strings will have the incremented value appended
  to the name).  Only one element of each type can be inserted into any specific snippet as it
  makes no sense to insert multiple elements of the same type - for those use cases the user
  must create a smaller snippet subset.

------------------------------------------------------------------------------------------*/

class GCSnippetsForm : public QDialog
{
  Q_OBJECT
  
public:
  /* QDomElement creates shallow copies by default so we will be manipulating the active
    DOM document directly via this dialog. */
  explicit GCSnippetsForm( const QString &elementName, QDomElement parentElement, QWidget *parent = 0 );
  ~GCSnippetsForm();

signals:
  void snippetAdded();

private slots:
  void setCurrentCheckBox( QWidget* checkBox );
  void elementSelected   ( QTreeWidgetItem *item, int column );
  void attributeChanged  ( QTableWidgetItem *item );
  void attributeValueChanged();
  void addSnippet();
  void showHelp();
  
private:
  void populateTreeWidget( const QString &elementName );
  void processNextElement( const QString &elementName, QTreeWidgetItem *parent, QDomNode parentNode );
  void updateCheckStates ( QTreeWidgetItem *item );

  void showErrorMessageBox( const QString &errorMsg );
  void deleteElementInfo();

  Ui::GCSnippetsForm *ui;
  QDomElement        *m_parentElement;
  QSignalMapper      *m_signalMapper;
  QCheckBox          *m_currentCheckBox;
  QDomDocument        m_domDoc;
  bool                m_treeItemActivated;

  QHash< QTreeWidgetItem*, GCDomElementInfo* > m_elementInfo;
  QHash< QTreeWidgetItem*, QDomElement >       m_treeItemNodes;

  QHash< QString /*attr*/, bool /*increment*/ > m_attributes;
  QHash< QString /*attr*/, QString /*value*/ >  m_originalValues;

};

#endif // GCSNIPPETSFORM_H
