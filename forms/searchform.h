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

#ifndef SEARCHFORM_H
#define SEARCHFORM_H

#include <QDialog>
#include <QPlainTextEdit>

namespace Ui {
class SearchForm;
}

class TreeWidgetItem;

/// Search through the current document for specific text.

/** The Qt::WA_DeleteOnClose flag is set for all instances of this form.  If
 * you're unfamiliar with Qt, this means that Qt will delete this widget as soon
 * as the widget accepts the close event (i.e. you don't need to worry about
 * clean-up of dynamically created instances of this object). */
class SearchForm : public QDialog {
  Q_OBJECT

public:
  /*! Constructor.
      @param elements - a list of all the elements in the active document.
      @param textEdit - the textEdit currently displaying the active document's
     DOM content. */
  explicit SearchForm(const QList<TreeWidgetItem *> &items,
                      QPlainTextEdit *textEdit, QWidget *parent = 0);

  /*! Destructor. */
  ~SearchForm();

signals:
  /*! Emitted when the search string is found in the document.  The item emitted
   * in this signal will contain the matched string in either its corresponding
   * element's name, or the name of an associated attribute or attribute value.
   */
  void foundItem(TreeWidgetItem *);

private slots:
  /*! Triggered when the user clicks the search button. If a match has already
   * been found, this function will search for the next occurrence of the search
   * string (if any). Repeatedly triggering this slot will cause the search
   * to cycle through all the matches in the user-specified direction. */
  void search();

  /*! Resets the text edit's cursor to the top or the bottom of the text edit
   * (depending on the direction of the search) so that the user can keep
   * cycling through all the matches (if any). */
  void resetCursor();

  /*! Triggered when the user ticks the "Search Up" checkbox. If checked, the
   * search will try to find a match above the current cursor position,
   * otherwise the search will try to find a match below the cursor position. */
  void searchUp();

  /*! Triggered when the user ticks the "Case Sensitive" checkbox. If checked,
   * the search string's case will be matched exactly. */
  void caseSensitive();

  /*! Triggered when the user ticks the "Whole Words Only" checkbox. If checked,
   * the search string's content will be matched exactly. */
  void wholeWords();

private:
  /*! Gathers all items matching "nodeText" into a list. */
  QList<TreeWidgetItem *> gatherMatchingItems(const QString &nodeText);

  /*! Called from within the search function, finds the match (if any) and emits
   * the foundItem signal if it does. */
  void findMatchingTreeItem(const QList<TreeWidgetItem *> matchingItems,
                            bool ascending);

  /*! Resets all "manually" highlighted text to their standard appearance
      \sa highlightFind
  */
  void resetHighlights();

  /*! Called whenever the found text does not match a node (i.e. matches text in
   * a closing bracket, or in a comment, etc) so that we may highlight the match
   * ourselves for display purposes. */
  void highlightFind();

  Ui::SearchForm *ui;
  QPlainTextEdit *m_text;
  QBrush m_savedBackground;
  QBrush m_savedForeground;
  bool m_wasFound;
  bool m_searchUp;
  bool m_firstRun;
  int m_previousIndex;

  QTextDocument::FindFlags m_searchFlags;
  QList<TreeWidgetItem *> m_items;
};

#endif // SEARCHFORM_H
