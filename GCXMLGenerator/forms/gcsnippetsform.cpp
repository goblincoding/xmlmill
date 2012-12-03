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

#include "gcsnippetsform.h"
#include "ui_gcsnippetsform.h"
#include "db/gcdatabaseinterface.h"
#include "utils/gccombobox.h"

#include <QCheckBox>
#include <QMessageBox>
#include <QDomDocument>
#include <QDomElement>
#include <QDomDocumentFragment>

/*--------------------------------------------------------------------------------------*/

GCSnippetsForm::GCSnippetsForm( const QString &elementName, QDomElement parentElement, QWidget *parent ) :
  QDialog         ( parent ),
  ui              ( new Ui::GCSnippetsForm ),
  m_parentElement ( &parentElement ),
  m_elementSnippet(),
  m_treeItemNodes (),
  m_elements      ()
{
  ui->setupUi( this );
  ui->tableWidget->setColumnWidth( 0, 40 );

  QDomDocument doc;
  m_elementSnippet = doc.createElement( elementName );

  populateTreeWidget( elementName );

  connect( ui->closeButton,    SIGNAL( clicked() ), this, SLOT( close() ) );
  connect( ui->addButton,      SIGNAL( clicked() ), this, SLOT( addSnippet() ) );
  connect( ui->showHelpButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );
  connect( ui->treeWidget,     SIGNAL( itemClicked  ( QTreeWidgetItem*, int ) ), this, SLOT( itemSelected( QTreeWidgetItem*, int ) ) );

  setAttribute( Qt::WA_DeleteOnClose );
}

/*--------------------------------------------------------------------------------------*/

GCSnippetsForm::~GCSnippetsForm()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::itemSelected( QTreeWidgetItem *item, int column )
{
  Q_UNUSED( column );

  resetTableWidget();

  /* Populate the table widget with the attributes and values associated with the element selected. */
  bool success( false );
  QDomElement *element = m_treeItemNodes.value( item );
  QString elementName = element->tagName();
  QStringList attributeNames = GCDataBaseInterface::instance()->attributes( elementName, success );

  /* This is more for debugging than for end-user functionality. */
  if( !success )
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
  }

  /* Create and add the "increment" checkbox to the first column of the table widget, add all the
    known attribute names to the cells in the second column of the table widget, create and populate
    combo boxes with the values associated with the attributes in question and insert the combo boxes
    into the third column of the table widget. */
  for( int i = 0; i < attributeNames.count(); ++i )
  {
    ui->tableWidget->setRowCount( i + 1 );

    QCheckBox *checkBox = new QCheckBox;
    ui->tableWidget->setCellWidget( i, 0, checkBox );
    connect( checkBox, SIGNAL( clicked() ), this, SLOT( valueChanged() ) );
    checkBox->setChecked( m_elements.value( elementName ).attr.value( attributeNames.at( i ) ).second );

    /* Items are editable by default, disable this option. */
    QTableWidgetItem *label = new QTableWidgetItem( attributeNames.at( i ) );
    label->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable );
    ui->tableWidget->setItem( i, 1, label );

    GCComboBox *attributeCombo = new GCComboBox;
    attributeCombo->addItems( GCDataBaseInterface::instance()->attributeValues( elementName, attributeNames.at( i ), success ) );
    attributeCombo->setEditable( true );
    attributeCombo->setCurrentIndex( attributeCombo->findText( m_elements.value( elementName ).attr.value( attributeNames.at( i ) ).first ) );
    connect( attributeCombo, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( valueChanged() ) );

    /* This is more for debugging than for end-user functionality. */
    if( !success )
    {
      showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
    }

    ui->tableWidget->setCellWidget( i, 2, attributeCombo );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::populateTreeWidget( const QString &elementName )
{
  ui->treeWidget->clear();
  resetTableWidget();

  QTreeWidgetItem *item = new QTreeWidgetItem;
  item->setText( 0, elementName );

  ui->treeWidget->invisibleRootItem()->addChild( item );  // takes ownership

  constructElement( elementName, m_elementSnippet, item );
  processNextElement( elementName, item );

  ui->treeWidget->expandAll();
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::processNextElement( const QString &elementName, QTreeWidgetItem *parent )
{
  bool success( false );
  QStringList children = GCDataBaseInterface::instance()->children( elementName, success );

  if( success )
  {
    foreach( QString child, children )
    {
      QTreeWidgetItem *item = new QTreeWidgetItem;
      item->setText( 0, child );

      parent->addChild( item );  // takes ownership

      constructElement( child, *m_treeItemNodes.value( item ), item );

      /* Since it isn't illegal to have elements with children of the same name, we cannot
        block it in the DB, however, if we DO have elements with children of the same name,
        this recursive call enters an infinite loop, so we need to make sure that doesn't
        happen. */
      if( child != elementName )
      {
        processNextElement( child, item );
      }
    }
  }
  else
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::setElementValues( const QString &elementName )
{
  Element element = m_elements.value( elementName );

  for( int i = 0; i < element.attr.size(); ++i )
  {
    /* First column. */
    QCheckBox *checkBox = dynamic_cast< QCheckBox* >( ui->tableWidget->cellWidget( i, 0 ) );

    /* Second column. */
    QString attributeName = ui->tableWidget->item( i, 1 )->text();

    /* Third column. */
    GCComboBox *comboBox = dynamic_cast< GCComboBox* >( ui->tableWidget->cellWidget( i, 2 ) );
    QString attributeValue = comboBox->currentText();

    element.attr.insert( attributeName, Value( attributeValue, checkBox->isChecked() ) );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::constructElement( const QString &elementName, QDomElement parentElement, QTreeWidgetItem *item )
{
  /* QDomElement's shallow copy constructor allows us to set up the relationships between
    the snippet's children and their children, etc etc.  This means that we can later manipulate
    the elements' data directly without messing with the hierarchy. */
  QDomDocument doc;
  QDomElement element = doc.createElement( elementName );

  bool success( false );
  QStringList attributeNames = GCDataBaseInterface::instance()->attributes( elementName, success );

  /* This is more for debugging than for end-user functionality. */
  if( !success )
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
  }

  for( int i = 0; i < attributeNames.count(); ++i )
  {
    element.setAttribute( attributeNames.at( i ), "" );
  }

  Element elemStruct;
  elemStruct.elem = &element;

  QDomNamedNodeMap attributeMap = element.attributes();

  for( int i = 0; i <  attributeMap.size(); ++i )
  {
    elemStruct.attr.insert( attributeMap.item( 0 ).toAttr().name(),
                            Value( attributeMap.item( 0 ).toAttr().value(), false ) );
  }

  m_elements.insert( elementName, elemStruct );

  m_treeItemNodes.insert( item, &element );

  if( !parentElement.isNull() )
  {
    parentElement.appendChild( element );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::resetTableWidget()
{
  ui->tableWidget->clearContents();   // also deletes current items
  ui->tableWidget->setRowCount( 0 );
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::valueChanged()
{
  /* Will reconsider keeping track of which exact value changed later, however, I don't
    think the extra functionality and complication will add significantly to this solution. */
  setElementValues( ui->treeWidget->currentItem()->text( 0 ) );
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::addSnippet()
{
  for( int i = 0; i < ui->spinBox->value(); ++i )
  {
    QHash< QString, Element >::const_iterator elemIt = m_elements.constBegin();

    while( elemIt != m_elements.constEnd() )
    {
      Element elemStruct = static_cast< Element >( elemIt.value() );
      QDomElement element( *elemStruct.elem );    //shallow copy

      QHash< QString, Value >::const_iterator attrIt = elemStruct.attr.constBegin();

      while( attrIt != elemStruct.attr.constEnd() )
      {
        Value val = static_cast< Value >( attrIt.value() );
        QString attributeValue = val.first;

        /* If the user wants us to increment the default value. */
        if( val.second )
        {
          /* Check if this is a number (if it contains any non-digit character). */
          if( !attributeValue.contains( "\\D" ) )
          {
            bool ok( false );
            int intValue = attributeValue.toInt( &ok );

            if( ok )
            {
              intValue += i;
              attributeValue = QString( "%1" ).arg( intValue );
            }
          }
          else
          {
            /* If the value contains some string characters, it's a string value and that's all
              there is to it. */
            attributeValue += QString( "%1" ).arg( i );
          }
        }

        element.setAttribute( attrIt.key(), attributeValue );
        attrIt++;
      }

      elemIt++;
    }

    if( !m_parentElement->isNull() )
    {
      m_parentElement->appendChild( m_elementSnippet.cloneNode() );
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::showHelp()
{
  QMessageBox::information( this,
                            "How this works...",
                            "Use this form to create XML snippets with the default values\n"
                            "you specify.\n\n"
                            "If you tick the \"Incr\" option in the first column, then the\n"
                            "default integer value will be incremented with \"1\" for however many\n"
                            "snippets you generate.\n\n"
                            "For example, if you specify \"10\" as an attribute value, then the \n"
                            "first snippet will assign \"10\" to the attribute in question,\n"
                            "the second will have \"11\", the third, \"12\", etc.\n\n"
                            "Strings will have the incremented value appended to the name, but all\n"
                            "other data types will be left as they are.\n" );
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::showErrorMessageBox( const QString &errorMsg )
{
  QMessageBox::critical( this, "Error!", errorMsg );
}

/*--------------------------------------------------------------------------------------*/
