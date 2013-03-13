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
  void selectedIndex( int );

private slots:
  /*! Activated when the cursor in the plain text edit changes. */
  void emitSelectedIndex();

private:
  bool m_cursorPositionChanging;
  
};

#endif // GCPLAINTEXTEDIT_H
