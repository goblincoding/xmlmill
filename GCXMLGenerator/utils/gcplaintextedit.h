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

#ifndef GCPLAINTEXTEDIT_H
#define GCPLAINTEXTEDIT_H

#include <QPlainTextEdit>
#include <QTextBlock>

/**
   Provides functionality with which to comment out or uncomment XML selections
   and keeps track of which XML nodes are currently under investigation (based
   on cursor positions).
*/

class GCPlainTextEdit : public QPlainTextEdit
{
  Q_OBJECT
public:
  explicit GCPlainTextEdit( QWidget *parent = 0 );

  /*! Use instead of "setPlainText" as it improves performance significantly (especially
      for larger documents). */
  void setContent( const QString &text );

  /*! Finds the "relativePos"'s occurrence of "text" within the active document. */
  void findTextRelativeToDuplicates( const QString &text, int relativePos );

  /*! Resets the internal state of GCPlainTextEdit. */
  void clearAndReset();

public slots:
  /*! Sets the necessary flags on the text edit to wrap or unwrap text as per user preference. */
  void wrapText( bool wrap );
  
signals:
  /*! Emitted when the user clicks or otherwise moves within the text edit.
      \sa emitSelectedIndex */
  void selectedIndex( int );

  /*! Emitted whenever a selection has been commented out. The parameter list contains
      the indices corresponding to the items that should be removed from the tree widget.
      \sa commentOutSelection */
  void commentOut( const QList< int >&, const QString& );

  /*! Emitted whenever a selection must be "uncommented". The parameter element corresponds to
      a DOM snippet created from the selection.
      \sa uncommentSelection */
  void uncomment( const QString& );

protected:
  /*! Re-implemented from QPlainTextEdit. */
  void keyPressEvent( QKeyEvent *e );

private slots:
  /*! Activated when the cursor in the plain text edit changes.
      \sa selectedIndex */
  void emitSelectedIndex();

  /*! Shows the default context menu with the additional options to "Comment Out Selection"
      and "Uncomment Selection"
      \sa commentOut
      \sa commentOutSelection
      \sa uncomment
      \sa uncommentSelection */
  void showContextMenu( const QPoint &point );

  /*! Comments out the selected text.  If the action broke the DOM, the user has the option
      to "Undo" and try again. If successful, the text is commented out and the "commentOut"
      signal is emitted.
      \sa commentOut */
  void commentOutSelection();

  /*! Uncomments a selection that's currently commented out.
      \sa uncomment */
  void uncommentSelection();

  /*! Check if the DOM got broken when the user commented out or uncommented sections. */
  bool confirmDomNotBroken();

  /*! Accounts for non-active document aspects (comments and element closing brackets),
      to determine the index corresponding to a specific block number. */
  int findIndexMatchingBlockNumber( QTextBlock block );

private:
  QBrush   m_savedPalette;
  QAction *m_comment;
  QAction *m_uncomment;
  bool m_undoAvailable;
  bool m_cursorPositionChanging;
  
};

#endif // GCPLAINTEXTEDIT_H
