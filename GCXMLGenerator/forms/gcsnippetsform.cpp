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
#include <QSignalMapper>

/*--------------------------------------------------------------------------------------*/

GCSnippetsForm::GCSnippetsForm( const QString &elementName, QDomElement parentElement, QWidget *parent ) :
  QDialog          ( parent ),
  ui               ( new Ui::GCSnippetsForm ),
  m_parentElement  ( &parentElement ),
  m_signalMapper   ( new QSignalMapper( this ) ),
  m_currentCheckBox( NULL ),
  m_domDoc         (),
  m_attributes     (),
  m_originalValues (),
  m_elements       (),
  m_checkBoxes     ()
{
  ui->setupUi( this );
  ui->tableWidget->setColumnWidth( 0, 40 );  // restricted for checkbox
  ui->treeWidget->setColumnWidth ( 0, 50 );  // restricted for checkbox

  connect( m_signalMapper, SIGNAL( mapped( QWidget* ) ), this, SLOT( setCurrentCheckBox( QWidget* ) ) );

  populateTreeWidget( elementName );

  connect( ui->closeButton,    SIGNAL( clicked() ), this, SLOT( close() ) );
  connect( ui->addButton,      SIGNAL( clicked() ), this, SLOT( addSnippet() ) );
  connect( ui->showHelpButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );
  connect( ui->treeWidget,     SIGNAL( itemClicked( QTreeWidgetItem*, int ) ), this, SLOT( treeWidgetItemSelected( QTreeWidgetItem*, int ) ) );

  setAttribute( Qt::WA_DeleteOnClose );
}

/*--------------------------------------------------------------------------------------*/

GCSnippetsForm::~GCSnippetsForm()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::treeWidgetItemSelected( QTreeWidgetItem *item, int column )
{
  ui->tableWidget->clearContents();   // also deletes current items
  ui->tableWidget->setRowCount( 0 );

  /* Populate the table widget with the attributes and values associated with the element selected. */
  bool success( false );
  QString elementName = item->text( column );
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
    connect( checkBox, SIGNAL( clicked() ), this, SLOT( attributeValueChanged() ) );
    checkBox->setChecked( m_attributes.value( elementName + attributeNames.at( i ) ) );

    /* Items are editable by default, disable this option. */
    QTableWidgetItem *label = new QTableWidgetItem( attributeNames.at( i ) );
    label->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable );
    ui->tableWidget->setItem( i, 1, label );

    GCComboBox *attributeCombo = new GCComboBox;
    attributeCombo->addItems( GCDataBaseInterface::instance()->attributeValues( elementName, attributeNames.at( i ), success ) );
    attributeCombo->setEditable( true );
    attributeCombo->setCurrentIndex( attributeCombo->findText(
                                       m_domDoc
                                       .elementsByTagName( elementName )
                                       .at( 0 ).namedItem( attributeNames.at( i ) )
                                       .toAttr().value() ) );
    connect( attributeCombo, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( attributeValueChanged() ) );

    /* This is more for debugging than for end-user functionality. */
    if( !success )
    {
      showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
    }

    ui->tableWidget->setCellWidget( i, 2, attributeCombo );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::setCurrentCheckBox( QWidget *checkBox )
{
  m_currentCheckBox = dynamic_cast< QCheckBox* >( checkBox );
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::attributeValueChanged()
{
  /* Update the element's attribute inclusions, values and value increment flags. */
  QString elementName = ui->treeWidget->currentItem()->text( 1 );

  /* There will only ever be one element with this name the way this class has been implemented. */
  QDomElement element = m_domDoc.elementsByTagName( elementName ).at( 0 ).toElement();

  QCheckBox *elemCheck = dynamic_cast< QCheckBox* >( ui->treeWidget->itemWidget( ui->treeWidget->currentItem(), 0 ) );
  m_elements.insert( elementName, elemCheck->isChecked() );

  QDomNamedNodeMap attributes = element.attributes();

  for( int i = 0; i < attributes.size(); ++i )
  {
    /* First column. */
    QCheckBox *checkBox = dynamic_cast< QCheckBox* >( ui->tableWidget->cellWidget( i, 0 ) );

    /* Second column. */
    QString attributeName = ui->tableWidget->item( i, 1 )->text();

    /* Third column. */
    GCComboBox *comboBox = dynamic_cast< GCComboBox* >( ui->tableWidget->cellWidget( i, 2 ) );
    QString attributeValue = comboBox->currentText();
    m_originalValues.insert( elementName + attributeName, attributeValue );

    element.setAttribute( attributeName, attributeValue );
    m_attributes.insert( elementName + attributeName, checkBox->isChecked() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::elementValueChanged()
{
  /* Replace the previous value (if any) of the element exclude flag. */
  QString elementName = m_checkBoxes.value( m_currentCheckBox );
  m_elements.insert( elementName, m_currentCheckBox->isChecked() );
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::addSnippet()
{
  /* Create a duplicate DOM doc so that we do not remove the elements in their
    entirety...when the user changes the snippet structure, the original DOM must
    be accessible (the DOM containing all known elements). */
  QDomDocument doc = m_domDoc.cloneNode().toDocument();

  QList< QString > elementNames = m_elements.keys();
  QHash< QString, bool >::const_iterator elemIt = m_elements.constBegin();

  while( elemIt != m_elements.constEnd() )
  {
    /* Remove the element if the user checked the box. */
    if( elemIt.value() )
    {
      /* There will only ever be one element with this name the way this class has been implemented. */
      QDomNode parent = doc.elementsByTagName( elemIt.key() ).at( 0 ).parentNode();
      parent.removeChild( doc.elementsByTagName( elemIt.key() ).at( 0 ) );
      elementNames.removeAll( elemIt.key() );
    }

    elemIt++;
  }

  /* Add the required number of snippets. */
  for( int i = 0; i < ui->spinBox->value(); ++i )
  {
    /* Update all the elements and attribute values. */
    for( int j = 0; j < elementNames.size(); ++j )
    {
      QString elementName = elementNames.at( j );

      /* There will only ever be one element with this name the way this class has been implemented. */
      QDomElement element = doc.elementsByTagName( elementName ).at( 0 ).toElement();
      QDomNamedNodeMap attributes = element.attributes();

      for( int k = 0; k < attributes.size(); ++k )
      {
        QDomAttr attr = attributes.item( k ).toAttr();

        if( m_attributes.value( elementName + attr.name() ) )
        {
          QString attributeValue = m_originalValues.value( elementName + attr.name() );

          /* Check if this is a number (if it contains any non-digit character). */
          if( !attributeValue.contains( QRegExp( "\\D+" ) ) )
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

          element.setAttribute( attr.name(), attributeValue );
        }
      }
    }

    m_parentElement->appendChild( doc.documentElement().cloneNode() );
    emit snippetAdded();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::populateTreeWidget( const QString &elementName )
{
  QTreeWidgetItem *item = new QTreeWidgetItem;
  item->setText( 1, elementName );
  ui->treeWidget->invisibleRootItem()->addChild( item );  // takes ownership

  processNextElement( elementName, item, m_domDoc );

  ui->treeWidget->expandAll();
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::processNextElement( const QString &elementName, QTreeWidgetItem *parent, QDomNode parentNode )
{
  bool success( false );

  QDomElement element = m_domDoc.createElement( elementName );
  m_elements.insert( elementName, false );

  /* This will only be the case the first time this function is called. */
  if( parentNode.isNull() )
  {
    m_domDoc.appendChild( element );
  }
  else
  {
    parentNode.appendChild( element );
  }

  QStringList attributeNames = GCDataBaseInterface::instance()->attributes( elementName, success );

  /* This is more for debugging than for end-user functionality. */
  if( !success )
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
  }

  /* Create all the possible attributes for the element here, they can be changed
    later on. */
  for( int i = 0; i < attributeNames.count(); ++i )
  {
    element.setAttribute( attributeNames.at( i ), "" );
  }

  QStringList children = GCDataBaseInterface::instance()->children( elementName, success );

  if( success )
  {
    foreach( QString child, children )
    {
      QTreeWidgetItem *item = new QTreeWidgetItem;
      item->setText( 1, child );
      parent->addChild( item );  // takes ownership

      QCheckBox *checkBox = new QCheckBox;
      ui->treeWidget->setItemWidget( item, 0, checkBox );
      m_checkBoxes.insert( checkBox, child );
      connect( checkBox, SIGNAL( clicked() ), this, SLOT( elementValueChanged() ) );

      connect( checkBox, SIGNAL( stateChanged( int ) ), m_signalMapper, SLOT( map() ) );
      m_signalMapper->setMapping( checkBox, checkBox );

      /* Since it isn't illegal to have elements with children of the same name, we cannot
        block it in the DB, however, if we DO have elements with children of the same name,
        this recursive call enters an infinite loop, so we need to make sure that doesn't
        happen. */
      if( child != elementName )
      {
        processNextElement( child, item, element );
      }
    }
  }
  else
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::showHelp()
{
  QMessageBox::information( this,
                            "How this works...",
                            "Use this form to create XML snippets with the default values\n"
                            "you specify.\n\n"
                            "Ticking \"Exclude\" next to any specific element will exclude it\n"
                            "(and all of its children) from the snippet.\n\n"
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
