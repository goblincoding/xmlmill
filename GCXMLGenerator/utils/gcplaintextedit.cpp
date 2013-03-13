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

#include "gcplaintextedit.h"
#include "xml/xmlsyntaxhighlighter.h"
#include "utils/gcglobalspace.h"

/*--------------------------------------------------------------------------------------*/

GCPlainTextEdit::GCPlainTextEdit( QWidget *parent ) :
  QPlainTextEdit( parent ),
  m_cursorPositionChanging( false )
{
  setAcceptDrops( false );
  setFont( QFont( GCGlobalSpace::FONT, GCGlobalSpace::FONTSIZE ) );
  setCenterOnScroll( true );

  connect( this, SIGNAL( cursorPositionChanged() ), this, SLOT( emitSelectedIndex() ) );

  /* Everything happens automagically and the text edit takes ownership. */
  XmlSyntaxHighlighter *highLighter = new XmlSyntaxHighlighter( document() );
  Q_UNUSED( highLighter );
}

/*--------------------------------------------------------------------------------------*/

void GCPlainTextEdit::setContent( const QString &text )
{
  m_cursorPositionChanging = true;

  /* Squeezing every once of performance out of the text edit...this significantly speeds
    up the loading of large files. */
  setUpdatesEnabled( false );
  setPlainText( text );
  setUpdatesEnabled( true );

  m_cursorPositionChanging = false;
}

/*--------------------------------------------------------------------------------------*/

void GCPlainTextEdit::findTextRelativeToDuplicates( const QString &text, int relativePos )
{
  m_cursorPositionChanging = true;

  moveCursor( QTextCursor::Start );

  for( int i = 0; i <= relativePos; ++i )
  {
    find( text );
  }

  m_cursorPositionChanging = false;
}

/*--------------------------------------------------------------------------------------*/

void GCPlainTextEdit::clearAndReset()
{
  m_cursorPositionChanging = true;
  clear();
  m_cursorPositionChanging = false;
}

/*--------------------------------------------------------------------------------------*/

void GCPlainTextEdit::emitSelectedIndex()
{
  if( !m_cursorPositionChanging )
  {
    int itemNumber = textCursor().blockNumber();
    bool insideComment = false;

    QTextBlock block = textCursor().block();

    while( block.isValid() && ( block.blockNumber() > 0 ) )
    {
      /* Check if we just entered a comment block (this is NOT wrong, remember
        that we are working our way back up the document, not down). */
      if( block.text().contains( "-->" ) )
      {
        insideComment = true;
      }

      if( insideComment || block.text().contains( "</" ) )
      {
        itemNumber--;
      }

      /* Check if we are about to exit a comment block. */
      if( block.text().contains( "<!--" ) )
      {
        insideComment = false;
      }

      block = block.previous();
    }

    emit selectedIndex( itemNumber );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCPlainTextEdit::wrapText( bool wrap )
{
  if( wrap )
  {
    setLineWrapMode( QPlainTextEdit::WidgetWidth );
  }
  else
  {
    setLineWrapMode( QPlainTextEdit::NoWrap );
  }
}

/*--------------------------------------------------------------------------------------*/

