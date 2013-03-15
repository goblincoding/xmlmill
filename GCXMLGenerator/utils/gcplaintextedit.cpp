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
#include "utils/gcmessagespace.h"

#include <QMenu>
#include <QAction>
#include <QDomDocument>

/*--------------------------------------------------------------------------------------*/

const QString OPENCOMMENT( "<!--" );
const QString CLOSECOMMENT( "-->" );

/*--------------------------------------------------------------------------------------*/

GCPlainTextEdit::GCPlainTextEdit( QWidget *parent ) :
  QPlainTextEdit  ( parent ),
  m_savedPalette  (),
  m_comment       ( NULL ),
  m_uncomment     ( NULL ),
  m_undoAvailable ( false ),
  m_cursorPositionChanging( false )
{
  setAcceptDrops( false );
  setFont( QFont( GCGlobalSpace::FONT, GCGlobalSpace::FONTSIZE ) );
  setCenterOnScroll( true );
  setTextInteractionFlags( Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard );
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
    emit selectedIndex( findIndexMatchingBlockNumber( textCursor().block() ) );
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
  m_cursorPositionChanging = true;

  int selectionStart = textCursor().selectionStart();
  int selectionEnd = textCursor().selectionEnd();

  QTextCursor cursor = textCursor();
  cursor.setPosition( selectionEnd );
  cursor.movePosition( QTextCursor::EndOfBlock );

  int finalBlockNumber = cursor.blockNumber();

  cursor.setPosition( selectionStart );
  cursor.movePosition( QTextCursor::StartOfBlock );

  QList< int > indices;
  QTextBlock block = cursor.block();

  while( block.isValid() &&
         block.blockNumber() <= finalBlockNumber )
  {
    QString blocktext = block.text();
    indices.append( findIndexMatchingBlockNumber( block ) );
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

  if( confirmDomNotBroken() )
  {
    emit commentOut( indices );
  }

  m_cursorPositionChanging = false;
}

/*--------------------------------------------------------------------------------------*/

void GCPlainTextEdit::uncommentSelection()
{
  m_cursorPositionChanging = true;

  QString selectedText = textCursor().selectedText();

  int selectionStart = textCursor().selectionStart();
  int selectionEnd = textCursor().selectionEnd();

  QTextCursor cursor = textCursor();
  cursor.setPosition( selectionStart );
  cursor.movePosition( QTextCursor::StartOfBlock );
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

  if( confirmDomNotBroken() )
  {
    selectedText.remove( OPENCOMMENT );
    selectedText.remove( CLOSECOMMENT );

    QDomDocument doc;
    doc.setContent( selectedText );
    emit uncomment( doc.documentElement().cloneNode().toElement() );
  }

  m_cursorPositionChanging = false;
}

/*--------------------------------------------------------------------------------------*/

bool GCPlainTextEdit::confirmDomNotBroken()
{
  QString xmlErr( "" );
  int     line  ( -1 );
  int     col   ( -1 );

  /* Create a temporary document so that we do not mess with the contents
    of the tree item node map and current DOM if the new XML is broken. */
  QDomDocument doc;

  if( !doc.setContent( toPlainText(), &xmlErr, &line, &col ) )
  {
    QString errorMsg = QString( "XML is broken - Error [%1], line [%2], column [%3].\n\n"
                                "Please \"Undo\" and try again." )
                       .arg( xmlErr )
                       .arg( line )
                       .arg( col );
    GCMessageSpace::showErrorMessageBox( this, errorMsg );

    /* Unfortunately the line number returned by the DOM doc doesn't match up with what's
      visible in the QTextEdit.  It seems as if it's mostly off by one line.  For now it's a
      fix, but will have to figure out how to make sure that we highlight the correct lines.
      Ultimately this finds the broken XML and highlights it in red...what a mission... */
    QTextBlock textBlock = document()->findBlockByLineNumber( line - 1 );
    QTextCursor cursor( textBlock );
    cursor.movePosition( QTextCursor::NextWord );
    cursor.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );

    m_savedPalette = cursor.blockCharFormat().background();

    QTextEdit::ExtraSelection highlight;
    highlight.cursor = cursor;
    highlight.format.setBackground( QColor( 220, 150, 220 ) );
    highlight.format.setProperty  ( QTextFormat::FullWidthSelection, true );

    QList< QTextEdit::ExtraSelection > extras;
    extras << highlight;
    setExtraSelections( extras );
    ensureCursorVisible();

    m_undoAvailable = true;
    return false;
  }

  return true;
}

/*--------------------------------------------------------------------------------------*/

int GCPlainTextEdit::findIndexMatchingBlockNumber( QTextBlock block )
{
  int itemNumber = block.blockNumber();
  int errorCounter = 0;
  bool insideComment = false;
  int otherBlockNumber = block.blockNumber();

  while( block.isValid() &&
         block.blockNumber() > 0 )
  {
    QString blockText = block.text();

    /* Check if we just entered a comment block (this is NOT wrong, remember
      that we are working our way back up the document, not down). */
    if( block.text().contains( CLOSECOMMENT ) )
    {
      errorCounter = 0;
      insideComment = true;
    }

    if( insideComment ||
        block.text().contains( "</" ) )
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

  return itemNumber;
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

void GCPlainTextEdit::keyPressEvent( QKeyEvent *e )
{
  if( e->matches( QKeySequence::Undo ) && m_undoAvailable )
  {
    /* Not a typo, comment opening and closing brackets are matching pairs, undo in one go. */
    undo();
    undo();

    QTextEdit::ExtraSelection highlight;
    highlight.cursor = textCursor();
    highlight.format.setBackground( m_savedPalette );
    highlight.format.setProperty  ( QTextFormat::FullWidthSelection, true );

    QList< QTextEdit::ExtraSelection > extras;
    extras << highlight;
    setExtraSelections( extras );
    m_undoAvailable = false;
  }
  else
  {
    QPlainTextEdit::keyPressEvent( e );
  }
}

/*--------------------------------------------------------------------------------------*/

