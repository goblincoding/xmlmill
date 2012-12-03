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

#include <QMessageBox>
#include <QDomElement>

/*--------------------------------------------------------------------------------------*/

GCSearchForm::GCSearchForm( const QList< QDomElement > &elements, const QString &docContents, QWidget *parent ) :
  QDialog      ( parent ),
  ui           ( new Ui::GCSearchForm ),
  m_text       (),
  m_wasFound   ( false ),
  m_searchFlags( 0 ),
  m_elements   ( elements )
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
  if( found != m_wasFound &&
      m_wasFound )
  {
    resetCursor();
    found = m_text.find( searchText, m_searchFlags );
  }

  if( found )
  {
    m_wasFound = true;

    /* Highlight the entire node - element, attributes and attribute values. */
    m_text.moveCursor( QTextCursor::StartOfLine );
    m_text.moveCursor( QTextCursor::EndOfLine, QTextCursor::KeepAnchor );

    /* Remove the special characters. */
    QString nodeText = m_text.textCursor().selectedText().remove( QRegExp( "<|>|\\/") ).trimmed();

    /* Extract the element name, attributes and attribute values. The element's
      name will always appear first. */
    QString elementName = nodeText.section( " ", 0, 0 );
    nodeText.remove( elementName );
    nodeText.trimmed();

    QHash< QString, QString > attributeMap;
    int nrAttValPairs = nodeText.count( "=" );

    for( int i = 0; i < nrAttValPairs; ++i )
    {
      /* Extract the attribute name and remove the name as well as the "=" sign
        from the node string. */
      QString attributeName = nodeText.mid( 0, nodeText.indexOf( "=" ) );
      nodeText.remove( 0, nodeText.indexOf( "=" ) + 1 );

      /* Extract the attribute value and remove the value as well as both "\"" from the
        node string. */
      nodeText.remove( 0, nodeText.indexOf( "\"" ) + 1 );
      QString attributeValue = nodeText.mid( 0, nodeText.indexOf( "\"", 1 ) );
      nodeText.remove( 0, nodeText.indexOf( attributeValue ) + attributeValue.length() + 1 );

      attributeMap.insert( attributeName.trimmed(), attributeValue.trimmed() );
    }

    /* Now that we found the exact element/attribute/values of the first successful
      hit of this search, we need to figure out which of the DOM elements it corresponds to. */
    for( int i = 0; i < m_elements.size(); ++i )
    {
      if( m_elements.at( i ).tagName() == elementName )
      {
        QDomNamedNodeMap attributeNodes = m_elements.at( i ).attributes();

        /* First ensure that this node has exactly the same number of attributes
          as what we expect. */
        if( attributeMap.keys().size() == attributeNodes.size() )
        {
          bool exactMatch( true );

          /* Now check that the attribute names we have match up with those of this node. */
          foreach( QString attributeName, attributeMap.keys() )
          {
            if( !attributeNodes.contains( attributeName ) )
            {
              exactMatch = false;
              break;
            }
            else
            {
              if( attributeMap.value( attributeName ) != attributeNodes.namedItem( attributeName ).toAttr().value() )
              {
                exactMatch = false;
                break;
              }
            }
          }

          if( exactMatch )
          {
            foundMatch( m_elements.at( i ) );
            break;
          }
        }
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
  /* Reset cursor so that we may keep cycling through. */
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

void GCSearchForm::foundMatch( const QDomElement &element )
{
  emit foundElement( element );

  if( ui->searchButton->text() == "Search" )
  {
    ui->searchButton->setText( "Next" );
  }
}

/*--------------------------------------------------------------------------------------*/
