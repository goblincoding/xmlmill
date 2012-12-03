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
#include <QDomElement>

namespace Ui
{
  class GCSnippetsForm;
}

class QTreeWidgetItem;

/*------------------------------------------------------------------------------------------

  This form allows the user to add multiple XML snippets of the same structure with whichever
  default values the user specifies.  It furthermore allows for the option to increment the
  default values for each snippet (i.e. if the user specifies "1" as an attribute value with
  the option to increment, then the next snippet generated will have "2" as the value for the
  same attribute and so on and so forth.  Strings will have the incremented value appended
  to the name).

------------------------------------------------------------------------------------------*/

class GCSnippetsForm : public QDialog
{
  Q_OBJECT
  
public:
  /* QDomElement creates shallow copies by default so we will be manipulating the active
    DOM document directly via this dialog. */
  explicit GCSnippetsForm( const QString &elementName, QDomElement parentElement, QWidget *parent = 0 );
  ~GCSnippetsForm();

private slots:
  void itemSelected( QTreeWidgetItem *item, int column );
  void addSnippet  ();
  void valueChanged();
  void showHelp    ();
  
private:
  void populateTreeWidget ( const QString &elementName );
  void processNextElement ( const QString &elementName, QTreeWidgetItem *parent );
  void setElementValues   ( const QString &elementName );
  void constructElement   ( const QString &elementName, QDomElement parentElement, QTreeWidgetItem *item );
  void resetTableWidget();

  void showErrorMessageBox( const QString &errorMsg );

  Ui::GCSnippetsForm *ui;
  QDomElement        *m_parentElement;
  QDomElement         m_elementSnippet;

  QHash< QTreeWidgetItem*, QDomElement* > m_treeItemNodes;

  typedef QPair< QString /*value*/, bool /*increment*/ > Value;

  struct Element
  {
    QDomElement *elem;
    QHash< QString /*name*/, Value > attr;
  };

  QHash< QString /*name*/, Element > m_elements;
};

#endif // GCSNIPPETSFORM_H
