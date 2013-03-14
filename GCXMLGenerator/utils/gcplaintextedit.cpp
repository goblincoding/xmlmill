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

#include <QMenu>
#include <QAction>

/*--------------------------------------------------------------------------------------*/

const QString OPENCOMMENT( "<!--" );
const QString CLOSECOMMENT( "-->" );

/*--------------------------------------------------------------------------------------*/

GCPlainTextEdit::GCPlainTextEdit( QWidget *parent ) :
  QPlainTextEdit( parent ),
  m_comment     ( NULL ),
  m_uncomment   ( NULL ),
  m_cursorPositionChanging( false )
{
  setAcceptDrops( false );
  setFont( QFont( GCGlobalSpace::FONT, GCGlobalSpace::FONTSIZE ) );
  setCenterOnScroll( true );
  setContextMenuPolicy( Qt::CustomContextMenu );

  m_comment = new QAction( "Comment Out", this );
  m_uncomment = new QAction( "Uncomment", this );

  connect( m_comment, SIGNAL( triggered() ), this, SLOT( commentOutSelection() ) );
  connect( m_uncomment, SIGNAL( triggered() ), this, SLOT( uncommentSelection() ) );

  connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), this, SLOT( showContextMenu( const QPoint& ) ) );
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
    int errorCounter = 0;
    bool insideComment = false;

    QTextBlock block = textCursor().block();

    while( block.isValid() &&
           block.blockNumber() > 0 )
    {
      /* Check if we just entered a comment block (this is NOT wrong, remember
        that we are working our way back up the document, not down). */
      if( block.text().contains( CLOSECOMMENT ) )
      {
        errorCounter = 0;
        insideComment = true;
      }

      if( insideComment || block.text().contains( "</" ) )
      {
        itemNumber--;
      }

      /* Check if we are about to exit a comment block. */
      if( block.text().contains( OPENCOMMENT ) )
      {
        /* If we are exiting but we never entered, then we need to compensate for the
          subtractions we've done erroneously. */
        if( !insideComment )
        {
          itemNumber -= errorCounter;
        }

        insideComment = false;
      }

      errorCounter++;
      block = block.previous();
    }

    emit selectedIndex( itemNumber );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCPlainTextEdit::showContextMenu( const QPoint &point )
{
  QMenu *menu = createStandardContextMenu();
  menu->addSeparator();
  menu->addAction( m_comment );
  menu->addAction( m_uncomment );
  menu->exec( mapToGlobal( point ) );
  delete menu;
}

/*--------------------------------------------------------------------------------------*/

void GCPlainTextEdit::commentOutSelection()
{
  int selectionStart = textCursor().selectionStart();
  int selectionEnd = textCursor().selectionEnd();

  QTextCursor cursor = textCursor();
  cursor.setPosition( selectionStart );
  cursor.movePosition( QTextCursor::StartOfBlock );

  QList< int > indices;
  bool insideComment = false;
  QTextBlock block = cursor.block();

  while( block.isValid() &&
         cursor.position() <= selectionEnd )
  {
    /* Check if we just entered a comment block. */
    if( block.text().contains( OPENCOMMENT ) )
    {
      insideComment = true;
    }

    if( !insideComment &&
        !block.text().contains( "</" ) )
    {
      indices.append( block.blockNumber() );
    }

    /* Check if we are about to exit a comment block. */
    if( block.text().contains( CLOSECOMMENT ) )
    {
      insideComment = false;
    }

    cursor.setPosition( block.position() );
    block = block.next();
  }

  cursor.setPosition( selectionStart );
  cursor.beginEditBlock();
  cursor.insertText( OPENCOMMENT );
  cursor.endEditBlock();

  cursor.setPosition( selectionEnd );
  cursor.movePosition( QTextCursor::EndOfBlock );
  cursor.beginEditBlock();
  cursor.insertText( CLOSECOMMENT );
  cursor.endEditBlock();

  setTextCursor( cursor );
  emit commentOut( indices );
}

/*--------------------------------------------------------------------------------------*/

void GCPlainTextEdit::uncommentSelection()
{
  int selectionStart = textCursor().selectionStart();
  int selectionEnd = textCursor().selectionEnd();

  QTextCursor cursor = textCursor();
  cursor.setPosition( selectionStart );
  cursor.movePosition( QTextCursor::StartOfBlock );

  QList< int > indices;
  bool insideComment = false;
  QTextBlock block = cursor.block();

  while( block.isValid() &&
         cursor.position() <= selectionEnd )
  {
    /* Check if we just entered a comment block. */
    if( block.text().contains( OPENCOMMENT ) )
    {
      insideComment = true;
    }

    if( !insideComment &&
        !block.text().contains( "</" ) )
    {
      indices.append( block.blockNumber() );
    }

    /* Check if we are about to exit a comment block. */
    if( block.text().contains( CLOSECOMMENT ) )
    {
      insideComment = false;
    }

    cursor.setPosition( block.position() );
    block = block.next();
  }

  cursor.setPosition( selectionStart );
  cursor.beginEditBlock();

  QString text = cursor.block().text();
  text.remove( OPENCOMMENT );

  cursor.select( QTextCursor::BlockUnderCursor );
  cursor.removeSelectedText();
  cursor.insertBlock();
  cursor.insertText( text );
  cursor.endEditBlock();

  cursor.setPosition( selectionEnd );
  cursor.movePosition( QTextCursor::PreviousBlock );
  cursor.beginEditBlock();

  text = cursor.block().text();
  text.remove( CLOSECOMMENT );

  cursor.select( QTextCursor::BlockUnderCursor );
  cursor.removeSelectedText();
  cursor.insertBlock();
  cursor.insertText( text );
  cursor.endEditBlock();

  setTextCursor( cursor );
  emit uncomment( indices );
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

