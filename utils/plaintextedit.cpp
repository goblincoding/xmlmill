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

#include "plaintextedit.h"
#include "xml/xmlsyntaxhighlighter.h"
#include "utils/globalspace.h"
#include "utils/messagespace.h"

#include <QMenu>
#include <QAction>
#include <QDomDocument>
#include <QApplication>

/*--------------------------------------------------------------------------------------*/

const QString OPENCOMMENT( "<!--" );
const QString CLOSECOMMENT( "-->" );

/*-------------------------------- NON MEMBER FUNCTIONS --------------------------------*/

void removeDuplicates( QList< int >& indices )
{
  for( int i = 0; i < indices.size(); ++i )
  {
    if( indices.count( indices.at( i ) ) > 1 )
    {
      int backup = indices.at( i );

      /* Remove all duplicates. */
      indices.removeAll( backup );

      /* Add one occurrence back. */
      indices.append( backup );
    }
  }
}

/*---------------------------------- MEMBER FUNCTIONS ----------------------------------*/

PlainTextEdit::PlainTextEdit( QWidget* parent )
: QPlainTextEdit   ( parent ),
  m_savedBackground(),
  m_savedForeground(),
  m_comment        ( NULL ),
  m_uncomment      ( NULL ),
  m_deleteSelection( NULL ),
  m_deleteEmptyRow ( NULL ),
  m_insertEmptyRow ( NULL ),
  m_cursorPositionChanging( false ),
  m_cursorPositionChanged ( false ),
  m_mouseDragEntered      ( false ),
  m_textEditClicked       ( false )
{
  setAcceptDrops( false );
  setFont( QFont( GlobalSpace::FONT, GlobalSpace::FONTSIZE ) );
  setCenterOnScroll( true );
  setTextInteractionFlags( Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard );
  setContextMenuPolicy( Qt::CustomContextMenu );

  m_comment = new QAction( "Comment Out Selection", this );
  m_uncomment = new QAction( "Uncomment Selection", this );
  m_deleteSelection = new QAction( "Delete Selection", this );
  m_deleteEmptyRow = new QAction( "Delete Empty Line", this );
  m_insertEmptyRow = new QAction( "Insert Empty Line", this );

  m_deleteEmptyRow->setShortcut( Qt::Key_Delete );
  m_insertEmptyRow->setShortcut( Qt::Key_Return );

  connect( m_comment, SIGNAL( triggered() ), this, SLOT( commentOutSelection() ) );
  connect( m_uncomment, SIGNAL( triggered() ), this, SLOT( uncommentSelection() ) );
  connect( m_deleteSelection, SIGNAL( triggered() ), this, SLOT( deleteSelection() ) );
  connect( m_deleteEmptyRow, SIGNAL( triggered() ), this, SLOT( deleteEmptyRow() ) );
  connect( m_insertEmptyRow, SIGNAL( triggered() ), this, SLOT( insertEmptyRow() ) );

  connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), this, SLOT( showContextMenu( const QPoint& ) ) );
  connect( this, SIGNAL( cursorPositionChanged() ), this, SLOT( setCursorPositionChanged() ) );

  /* Everything happens automagically and the text edit takes ownership. */
  XmlSyntaxHighlighter* highLighter = new XmlSyntaxHighlighter( document() );
  Q_UNUSED( highLighter )
  ;
}

/*--------------------------------------------------------------------------------------*/

void PlainTextEdit::setContent( const QString& text )
{
  m_cursorPositionChanging = true;

  /* Squeezing every ounce of performance out of the text edit...this significantly speeds
    up the loading of large files. */
  setUpdatesEnabled( false );
  setPlainText( text );
  setUpdatesEnabled( true );

  m_cursorPositionChanging = false;
}

/*--------------------------------------------------------------------------------------*/

void PlainTextEdit::findTextRelativeToDuplicates( const QString& text, int relativePos )
{
  /* If the user clicked on any element's representation in the text edit, then there is
     no need to find the text (this method is called after the tree is updated with the
     selected element) since we already know where it is and moving the cursor around
     will only make the text edit "jump" positions.  Rather highlight the text ourselves.
    */
  if( m_textEditClicked )
  {
    m_savedBackground = textCursor().blockCharFormat().background();
    m_savedForeground = textCursor().blockCharFormat().foreground();

    QTextEdit::ExtraSelection extra;
    extra.cursor = textCursor();
    extra.format.setProperty( QTextFormat::FullWidthSelection, true );
    extra.format.setBackground( QApplication::palette().highlight() );
    extra.format.setForeground( QApplication::palette().highlightedText() );

    QList< QTextEdit::ExtraSelection > extras;
    extras << extra;
    setExtraSelections( extras );
    m_textEditClicked = false;
  }
  else
  {
    /* Unset any previously set selections. */
    QList< QTextEdit::ExtraSelection > extras = extraSelections();

    for( int i = 0; i < extras.size(); ++i )
    {
      extras[ i ].format.setProperty( QTextFormat::FullWidthSelection, true );
      extras[ i ].format.setBackground( m_savedBackground );
      extras[ i ].format.setForeground( m_savedForeground );
    }

    setExtraSelections( extras );

    m_cursorPositionChanging = true;

    moveCursor( QTextCursor::Start );

    for( int i = 0; i <= relativePos; ++i )
    {
      find( text );
    }

    m_cursorPositionChanging = false;
  }
}

/*--------------------------------------------------------------------------------------*/

void PlainTextEdit::clearAndReset()
{
  m_cursorPositionChanging = true;
  clear();
  m_cursorPositionChanging = false;
}

/*--------------------------------------------------------------------------------------*/

void PlainTextEdit::emitSelectedIndex()
{
  if( !m_cursorPositionChanging )
  {
    emit selectedIndex( findIndexMatchingBlockNumber( textCursor().block() ) );
  }
}

/*--------------------------------------------------------------------------------------*/

void PlainTextEdit::setCursorPositionChanged()
{
  m_cursorPositionChanged = true;
}

/*--------------------------------------------------------------------------------------*/

void PlainTextEdit::showContextMenu( const QPoint& point )
{
  m_comment->setEnabled( textCursor().hasSelection() );
  m_uncomment->setEnabled( textCursor().hasSelection() );
  m_deleteSelection->setEnabled( textCursor().hasSelection() );

  QMenu* menu = createStandardContextMenu();
  menu->addSeparator();
  menu->addAction( m_comment );
  menu->addAction( m_uncomment );
  menu->addSeparator();
  menu->addAction( m_deleteSelection );
  menu->addAction( m_deleteEmptyRow );
  menu->addAction( m_insertEmptyRow );
  menu->exec( mapToGlobal( point ) );
  delete menu;
}

/*--------------------------------------------------------------------------------------*/

void PlainTextEdit::commentOutSelection()
{
  m_cursorPositionChanging = true;

  /* Capture the text before we make any changes. */
  QString comment = textCursor().selectedText();

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

  if( confirmDomNotBroken( 2 ) )
  {
    comment = comment.replace( QChar( 0x2029 ), '\n' );    // replace Unicode end of line character
    comment = comment.trimmed();
    removeDuplicates( indices );
    emit commentOut( indices, comment );
  }

  m_cursorPositionChanging = false;
}

/*--------------------------------------------------------------------------------------*/

void PlainTextEdit::uncommentSelection()
{
  m_cursorPositionChanging = true;

  int selectionStart = textCursor().selectionStart();
  int selectionEnd = textCursor().selectionEnd();

  QTextCursor cursor = textCursor();
  cursor.setPosition( selectionStart );
  cursor.movePosition( QTextCursor::StartOfBlock );

  cursor.setPosition( selectionEnd, QTextCursor::KeepAnchor );
  cursor.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );

  /* We need to capture this text way in the beginning before we start
    messing with cursor positions, etc. */
  QString selectedText = cursor.selectedText();

  cursor.beginEditBlock();
  cursor.removeSelectedText();
  selectedText.remove( OPENCOMMENT );
  selectedText.remove( CLOSECOMMENT );
  cursor.insertText( selectedText );
  cursor.endEditBlock();

  setTextCursor( cursor );

  m_cursorPositionChanging = false;

  if( confirmDomNotBroken( 2 ) )
  {
    emit manualEditAccepted();

    QTextCursor reselectCursor = textCursor();
    reselectCursor.setPosition( selectionStart );
    setTextCursor( reselectCursor );
    emitSelectedIndex();
  }
}

/*--------------------------------------------------------------------------------------*/

void PlainTextEdit::deleteSelection()
{
  m_cursorPositionChanging = true;

  textCursor().removeSelectedText();

  if( confirmDomNotBroken( 1 ) )
  {
    emit manualEditAccepted();
    emitSelectedIndex();
  }

  m_cursorPositionChanging = false;
}

/*--------------------------------------------------------------------------------------*/

void PlainTextEdit::insertEmptyRow()
{
  QTextCursor cursor = textCursor();
  cursor.movePosition( QTextCursor::EndOfBlock );
  cursor.insertBlock();
  setTextCursor( cursor );
}

/*--------------------------------------------------------------------------------------*/

void PlainTextEdit::deleteEmptyRow()
{
  QTextCursor cursor = textCursor();
  QTextBlock block = cursor.block();

  /* Check if the user is deleting an empty line (the only kind of deletion
    that is allowed). */
  if( block.text().remove( " " ).isEmpty() )
  {
    cursor.movePosition( QTextCursor::PreviousBlock );
    cursor.movePosition( QTextCursor::EndOfBlock );
    cursor.movePosition( QTextCursor::NextBlock, QTextCursor::KeepAnchor );
    cursor.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );
    cursor.removeSelectedText();
    setTextCursor( cursor );
  }
}

/*--------------------------------------------------------------------------------------*/

bool PlainTextEdit::confirmDomNotBroken( int undoCount )
{
  QString xmlErr( "" );
  int line  ( -1 );
  int col   ( -1 );

  /* Create a temporary document so that we do not mess with the contents
    of the tree item node map and current DOM if the new XML is broken. */
  QDomDocument doc;

  if( !doc.setContent( toPlainText(), &xmlErr, &line, &col ) )
  {
    /* Unfortunately the line number returned by the DOM doc doesn't match up with what's
      visible in the QTextEdit.  It seems as if it's mostly off by one line.  For now it's a
      fix, but will have to figure out how to make sure that we highlight the correct lines.
      Ultimately this finds the broken XML and highlights it in red...what a mission... */
    QTextBlock textBlock = document()->findBlockByLineNumber( line - 1 );
    QTextCursor cursor( textBlock );
    cursor.movePosition( QTextCursor::NextWord );
    cursor.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );

    m_savedBackground = cursor.blockCharFormat().background();

    QTextEdit::ExtraSelection highlight;
    highlight.cursor = cursor;
    highlight.format.setBackground( QColor( 220, 150, 220 ) );
    highlight.format.setProperty( QTextFormat::FullWidthSelection, true );

    QList< QTextEdit::ExtraSelection > extras;
    extras << highlight;
    setExtraSelections( extras );
    ensureCursorVisible();

    QString errorMsg = QString( "XML is broken - Error [%1], line [%2], column [%3].\n\n"
                                "Your action will be reverted." )
                                .arg( xmlErr )
                                .arg( line )
                                .arg( col );

    MessageSpace::showErrorMessageBox( this, errorMsg );

    for( int i = 0; i < undoCount; ++i )
    {
      undo();
    }

    highlight.cursor = textCursor();
    highlight.format.setBackground( m_savedBackground );
    highlight.format.setProperty( QTextFormat::FullWidthSelection, true );

    extras.clear();
    extras << highlight;
    setExtraSelections( extras );
    return false;
  }

  return true;
}

/*--------------------------------------------------------------------------------------*/

int PlainTextEdit::findIndexMatchingBlockNumber( QTextBlock block )
{
  int itemNumber = block.blockNumber();
  int errorCounter = 0;
  bool insideComment = false;

  while( block.isValid() &&
         block.blockNumber() >= 0 )
  {
    /* Check if we just entered a comment block (this is NOT wrong, remember
      that we are working our way back up the document, not down). */
    if( block.text().contains( CLOSECOMMENT ) )
    {
      errorCounter = 0;
      insideComment = true;
    }

    if( insideComment ||
        block.text().contains( "</" ) ||          // element close
        block.text().remove( " " ).isEmpty() ||   // empty lines
        ( block.text().contains( "<?" ) &&
          block.text().contains( "?>" ) ) )       // xml version specification
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

void PlainTextEdit::wrapText( bool wrap )
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

void PlainTextEdit::keyPressEvent( QKeyEvent* e )
{
  switch( e->key() )
  {
    case Qt::Key_Return:
      insertEmptyRow();
      break;
    case Qt::Key_Delete:
      deleteEmptyRow();
      break;
    default:
      QPlainTextEdit::keyPressEvent( e );
  }
}

/*--------------------------------------------------------------------------------------*/

void PlainTextEdit::mouseMoveEvent( QMouseEvent* e )
{
  m_mouseDragEntered = true;
  QPlainTextEdit::mouseMoveEvent( e );
}

/*--------------------------------------------------------------------------------------*/

void PlainTextEdit::mouseReleaseEvent( QMouseEvent* e )
{
  if( !m_mouseDragEntered &&
       m_cursorPositionChanged )
  {
    m_textEditClicked = true;
    emitSelectedIndex();
  }

  m_mouseDragEntered = false;
  m_cursorPositionChanged = false;
  QPlainTextEdit::mouseReleaseEvent( e );
}

/*--------------------------------------------------------------------------------------*/
