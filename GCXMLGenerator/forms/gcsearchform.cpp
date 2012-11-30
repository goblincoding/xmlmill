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

GCSearchForm::GCSearchForm( const QList< QDomElement > &elements, QWidget *parent ) :
  QWidget    ( parent ),
  ui         ( new Ui::GCSearchForm ),
  m_lastIndex( 0 ),
  m_elements ( elements )
{
  ui->setupUi( this );
  ui->lineEdit->setFocus();

  connect( ui->searchButton, SIGNAL( clicked() ), this, SLOT( search() ) );
  connect( ui->closeButton,  SIGNAL( clicked() ), this, SLOT( close() ) );
  connect( ui->lineEdit,     SIGNAL( returnPressed() ), this, SLOT( search() ) );

  setAttribute( Qt::WA_DeleteOnClose );
}

/*--------------------------------------------------------------------------------------*/

GCSearchForm::~GCSearchForm()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCSearchForm::search()
{
  QString searchText = ui->lineEdit->text();
  bool found = false;

  for( int i = m_lastIndex; i < m_elements.size(); ++i )
  {
    if( ui->elementCheckBox->isChecked() )
    {
      if( m_elements.at( i ).tagName() == searchText )
      {
        emit foundElement( m_elements.at( i ) );

        if( ui->searchButton->text() == "Search" )
        {
          ui->searchButton->setText( "Next" );
        }

        found = true;
      }

      if( ui->attributeCheckBox->isChecked() || ui->valueCheckBox->isChecked() )
      {
        QDomNamedNodeMap attributes = m_elements.at( i ).attributes();

        for( int j = 0; j < attributes.size(); ++j )
        {
          QDomAttr attribute = attributes.item( j ).toAttr();

          if( attribute.name() == searchText || attribute.value() == searchText )
          {
            emit foundElement( m_elements.at( i ) );

            if( ui->searchButton->text() == "Search" )
            {
              ui->searchButton->setText( "Next" );
            }

            found = true;
            break;
          }
        }
      }
    }

    m_lastIndex = i + 1;

    if( found )
    {
      break;
    }
  }

  /* Reset index so that we may keep cycling through. */
  if( m_lastIndex == m_elements.size() )
  {
    QMessageBox::information( this, "Reached the End", "Search reached end of document, continuing at the beginning." );
    m_lastIndex = 0;
  }
}

/*--------------------------------------------------------------------------------------*/
