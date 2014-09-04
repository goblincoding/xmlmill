/* Copyright (c) 2012 - 2013 by William Hallatt.
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

#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include <QPlainTextEdit>
#include <QTextBlock>

/// Specialist text edit class for displaying XML content in the XML Mill context.

/**
   Provides functionality with which to comment out or uncomment XML selections
   and keeps track of which XML nodes are currently under investigation (based
   on cursor positions).
*/

class PlainTextEdit : public QPlainTextEdit
{
Q_OBJECT
public:
  explicit PlainTextEdit( QWidget* parent = 0 );

  /*! Use instead of "setPlainText" as it improves performance significantly (especially
      for larger documents). */
  void setContent( const QString& text );

  /*! Finds the "relativePos"'s occurrence of "text" within the active document (i.e if there
      are multiple occurrences of "text" within the document, we will find the "relativePos"
      occurrence measured from the start of the document). */
  void findTextRelativeToDuplicates( const QString& text, int relativePos );

  /*! Resets the internal state of PlainTextEdit. */
  void clearAndReset();

public slots:
  /*! Sets the necessary flags on the text edit to wrap or unwrap text as per "wrap". */
  void wrapText( bool wrap );

signals:
  /*! Emitted whenever the selected index changes ("index" is the position of the text element/line
      relative to the first active element/beginning of the document, excluding comment blocks and
      other "non-active" XML).
      \sa emitSelectedIndex */
  void selectedIndex( int index );

  /*! Emitted whenever a selection has been commented out. The parameter list contains
      the indices corresponding to the items that should be removed from the tree widget. */
  void commentOut( const QList< int >&, const QString& );

  /*! Emitted whenever direct editing to the XML text occurred where the edit could represent any
      number of element additions/removals. */
  void manualEditAccepted();

protected:
  /*! Re-implemented from QPlainTextEdit. */
  void keyPressEvent( QKeyEvent* e );

  /*! Re-implemented from QPlainTextEdit. */
  void mouseMoveEvent( QMouseEvent* e );

  /*! Re-implemented from QPlainTextEdit. */
  void mouseReleaseEvent( QMouseEvent* e );

private slots:
  /*! Checks if the text cursor is currently being manipulated programmatically and emits
      the "selectedIndex" signal if appropriate.
      \sa selectedIndex */
  void emitSelectedIndex();

  /*! Activated when the cursorPositionChanged signal is emitted. */
  void setCursorPositionChanged();

  /*! Shows the default context menu with the custom actions appended.
      \sa commentOut
      \sa commentOutSelection
      \sa uncommentSelection
      \sa manualEditAccepted */
  void showContextMenu( const QPoint& point );

  /*! Comments out the selected text.  If successful, the text is commented out and the "commentOut"
      signal is emitted.
      \sa commentOut */
  void commentOutSelection();

  /*! Uncomments a selection that's currently commented out and broadcasts the general change notification
      via \sa manualEditAccepted */
  void uncommentSelection();

  /*! Permanently deletes the selection and broadcasts the general change notification
  via \sa manualEditAccepted */
  void deleteSelection();

  /*! Adds an empty row (useful when improving XML legibility).
      \sa deleteEmptyRow */
  void insertEmptyRow();

  /*! Deletes an empty row (useful when improving XML legibility).
      \sa insertEmptyRow */
  void deleteEmptyRow();

  /*! Check if the DOM was broken during a manual edit. */
  bool confirmDomNotBroken( int undoCount );

  /*! Finds the position (index) of the text element/line represented by "block" relative to the first active
      element of the document, excluding comment blocks and other "non-active" XML. */
  int findIndexMatchingBlockNumber( QTextBlock block );

private:
  QBrush m_savedBackground;
  QBrush m_savedForeground;
  QAction* m_comment;
  QAction* m_uncomment;
  QAction* m_deleteSelection;
  QAction* m_deleteEmptyRow;
  QAction* m_insertEmptyRow;
  bool m_cursorPositionChanging;
  bool m_cursorPositionChanged;
  bool m_mouseDragEntered;
  bool m_textEditClicked;
};

#endif // PLAINTEXTEDIT_H