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

#include "gcsearchform.h"
#include "ui_gcsearchform.h"
#include "utils/gctreewidgetitem.h"

#include <QMessageBox>

/*--------------------------------------------------------------------------------------*/

GCSearchForm::GCSearchForm( const QList< GCTreeWidgetItem * > &items, const QString &docContents, QWidget *parent ) :
  QDialog        ( parent ),
  ui             ( new Ui::GCSearchForm ),
  m_text         (),
  m_wasFound     ( false ),
  m_previousIndex( -1 ),
  m_searchFlags  ( 0 ),
  m_items        ( items )
{
  ui->setupUi( this );
  ui->lineEdit->setFocus();
  m_text.setText( docContents );

  connect( ui->searchButton, SIGNAL( clicked() ), this, SLOT( search() ) );
  connect( ui->closeButton,  SIGNAL( clicked() ), this, SLOT( close() ) );

  connect( ui->caseSensitiveCheckBox, SIGNAL( clicked() ), this, SLOT( caseSensitive() ) );
  connect( ui->wholeWordsCheckBox,    SIGNAL( clicked() ), this, SLOT( wholeWords() ) );
  connect( ui->searchUpCheckBox,      SIGNAL( clicked() ), this, SLOT( searchUp() ) );

  setAttribute( Qt::WA_DeleteOnClose );
}

/*--------------------------------------------------------------------------------------*/

GCSearchForm::~GCSearchForm()
{
  /* The QDomDocument m_doc points at is owned externally. */
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCSearchForm::search()
{
  QString searchText = ui->lineEdit->text();
  bool found = m_text.find( searchText, m_searchFlags );

  /* The first time we enter this function, if the text does not exist
    within the document, "found" and "m_wasFound" will both be false.
    However, if the text does exist, we wish to know that we found a match
    at least once so that, when we reach the end of the document and "found"
    is once more false, we can reset all indices and flags in order to start
    again from the beginning. */
  if( found != m_wasFound && m_wasFound )
  {
    resetCursor();
    found = m_text.find( searchText, m_searchFlags );
  }

  if( found )
  {
    m_wasFound = true;

    /* Highlight the entire node (element, attributes and attribute values) 
      in which the match was found. */
    m_text.moveCursor( QTextCursor::StartOfLine );
    m_text.moveCursor( QTextCursor::EndOfLine, QTextCursor::KeepAnchor );

    QString nodeText = m_text.textCursor().selectedText().trimmed();
    QList< GCTreeWidgetItem* > matchingItems;

    /* Find the first tree widget item corresponding to an element of name "elementName" */
    for( int i = 0; i < m_items.size(); ++i )
    {
      GCTreeWidgetItem* treeItem = m_items.at( i );

      if( m_items.at( i )->toString() == nodeText )
      {
        matchingItems.append( treeItem );
      }
    }

    qSort( matchingItems.begin(), matchingItems.end() );

    for( int i = 0; i <= matchingItems.size(); ++i )
    {
      GCTreeWidgetItem *treeItem = matchingItems.at( i );

      if( treeItem->index() > m_previousIndex )
      {
        m_previousIndex = treeItem->index();
        emit foundMatch( treeItem );
        break;
      }
    }
  }
  else
  {
    QMessageBox::information( this, "Not Found", QString( "Can't find the text:\"%1\"" ).arg( searchText ) );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSearchForm::resetCursor()
{
  /* Reset cursor so that we may keep cycling through the document content. */
  if( ui->searchUpCheckBox->isChecked() )
  {
    m_text.moveCursor( QTextCursor::End );
    QMessageBox::information( this, "Reached Top", "Search reached top, continuing at bottom." );
  }
  else
  {
    m_text.moveCursor( QTextCursor::Start );
    QMessageBox::information( this, "Reached Bottom", "Search reached bottom, continuing at top." );
  }

  m_previousIndex = -1;
}

/*--------------------------------------------------------------------------------------*/

void GCSearchForm::searchUp()
{
  /* Reset found flag every time the user changes the search options. */
  m_wasFound = false;

  if( ui->searchUpCheckBox->isChecked() )
  {
    m_searchFlags |= QTextDocument::FindBackward;
  }
  else
  {
    m_searchFlags ^= QTextDocument::FindBackward;
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSearchForm::caseSensitive()
{
  /* Reset found flag every time the user changes the search options. */
  m_wasFound = false;

  if( ui->caseSensitiveCheckBox->isChecked() )
  {
    m_searchFlags |= QTextDocument::FindCaseSensitively;
  }
  else
  {
    m_searchFlags ^= QTextDocument::FindCaseSensitively;
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSearchForm::wholeWords()
{
  /* Reset found flag every time the user changes the search options. */
  m_wasFound = false;

  if( ui->wholeWordsCheckBox->isChecked() )
  {
    m_searchFlags |= QTextDocument::FindWholeWords;
  }
  else
  {
    m_searchFlags ^= QTextDocument::FindWholeWords;
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSearchForm::foundMatch( GCTreeWidgetItem *treeItem )
{
  emit foundItem( treeItem );

  if( ui->searchButton->text() == "Search" )
  {
    ui->searchButton->setText( "Next" );
  }
}

/*--------------------------------------------------------------------------------------*/
