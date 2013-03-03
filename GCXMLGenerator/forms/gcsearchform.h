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

#ifndef GCSEARCHFORM_H
#define GCSEARCHFORM_H

#include <QDialog>
#include <QTextEdit>

namespace Ui
{
  class GCSearchForm;
}

class GCTreeWidgetItem;

/// Search through the current document for a specific element/attribute/value.

/** The Qt::WA_DeleteOnClose flag is set for all instances of this form.  If you're
    unfamiliar with Qt, this means that Qt will delete this widget as soon as the widget
    accepts the close event (i.e. you don't need to worry about clean-up of dynamically
    created instances of this object).
*/
class GCSearchForm : public QDialog
{
  Q_OBJECT
  
public:
  /*! Constructor.
      @param elements - a list of all the elements in the active document.
      @param docContents - the string representation of the active document's DOM content. */
  explicit GCSearchForm( const QList< GCTreeWidgetItem* > &items, const QString &docContents, QWidget *parent = 0 );

  /*! Destructor. */
  ~GCSearchForm();

signals:
  /*! Emitted when the search string is found in the document.  The element emitted along with this 
      signal will contain the matched string in either its name, or the name of an associated attribute 
      or an attribute value. */
  void foundItem( GCTreeWidgetItem *treeItem );

private slots:
  /*! Triggered when the user clicks the search button. If a match has already been found,
      this function will search for the next occurrence of the search string (if any). 
      Repeatedly triggering this slot will cause the search to cycle through all the
      matches in the user-specified direction. */
  void search();

  /*! Resets the text edit's cursor to the top or the bottom of the text edit (depending
      on the direction of the search) so that the user can keep cycling through all the
      matches (if any). */
  void resetCursor();

  /*! Triggered when the user ticks the "Search Up" checkbox. If checked, the search
      will try to find a match above the current cursor position, otherwise the search will
      try to find a match below the cursor position. */
  void searchUp();

  /*! Triggered when the user ticks the "Case Sensitive" checkbox. If checked, the search
      string's case will be matched exactly. */
  void caseSensitive();

  /*! Triggered when the user ticks the "Whole Words Only" checkbox. If checked, the search
      string's content will be matched exactly. */
  void wholeWords();
  
private:
  /*! Called from within the search function when a match is found and emits the foundItem
      signal. */
  void foundMatch( GCTreeWidgetItem *treeItem );

  Ui::GCSearchForm *ui;
  QTextEdit         m_text;
  bool              m_wasFound;
  int               m_previousIndex;

  QTextDocument::FindFlags m_searchFlags;
  QList< GCTreeWidgetItem* > m_items;

};

#endif // GCSEARCHFORM_H
