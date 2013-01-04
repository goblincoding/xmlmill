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
#include "utils/gcdomelementinfo.h"

#include <QCheckBox>
#include <QMessageBox>
#include <QDomDocument>
#include <QSignalMapper>
#include <QTreeWidgetItemIterator>

/*--------------------------------------------------------------------------------------*/

const int LABELCOLUMN = 0;
const int COMBOCOLUMN = 1;
const int INCRCOLUMN  = 2;

/*--------------------------------------------------------------------------------------*/

GCSnippetsForm::GCSnippetsForm( const QString &elementName, QDomElement parentElement, QWidget *parent ) :
  QDialog            ( parent ),
  ui                 ( new Ui::GCSnippetsForm ),
  m_parentElement    ( &parentElement ),
  m_domDoc           (),
  m_treeItemActivated( false ),
  m_elementInfo      (),
  m_treeItemNodes    (),
  m_attributes       (),
  m_originalValues   ()
{
  ui->setupUi( this );
  ui->tableWidget->setColumnWidth( INCRCOLUMN, 40 );  // restricted for checkbox
  ui->treeWidget->setColumnWidth ( 0, 50 );  // restricted for checkbox

  populateTreeWidget( elementName );

  connect( ui->closeButton,    SIGNAL( clicked() ), this, SLOT( close() ) );
  connect( ui->addButton,      SIGNAL( clicked() ), this, SLOT( addSnippet() ) );
  connect( ui->showHelpButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );
  connect( ui->treeWidget,     SIGNAL( itemClicked( QTreeWidgetItem*, int ) ), this, SLOT( elementSelected( QTreeWidgetItem*, int ) ) );
  connect( ui->tableWidget,    SIGNAL( itemChanged( QTableWidgetItem* ) ),     this, SLOT( attributeChanged( QTableWidgetItem* ) ) );

  setAttribute( Qt::WA_DeleteOnClose );
}

/*--------------------------------------------------------------------------------------*/

GCSnippetsForm::~GCSnippetsForm()
{
  deleteElementInfo();
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::elementSelected( QTreeWidgetItem *item, int column )
{
  m_treeItemActivated = true;

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
    ui->tableWidget->setCellWidget( i, INCRCOLUMN, checkBox );
    connect( checkBox, SIGNAL( clicked() ), this, SLOT( attributeValueChanged() ) );
    checkBox->setChecked( m_attributes.value( elementName + attributeNames.at( i ) ) );

    /* Items are editable by default, disable this option. */
    QTableWidgetItem *label = new QTableWidgetItem( attributeNames.at( i ) );
    label->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable );
    ui->tableWidget->setItem( i, LABELCOLUMN, label );

    GCComboBox *attributeCombo = new GCComboBox;
    attributeCombo->addItems( GCDataBaseInterface::instance()->attributeValues( elementName, attributeNames.at( i ), success ) );
    attributeCombo->setEditable( true );
    attributeCombo->setCurrentIndex( attributeCombo->findText(
                                       m_domDoc
                                       .elementsByTagName( elementName )
                                       .at( 0 )
                                       .toElement()
                                       .attribute( attributeNames.at( i ) ) ) );

    connect( attributeCombo, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( attributeValueChanged() ) );

    if( m_elementInfo.value( item )->includedAttributes().contains( attributeNames.at( i ) ) )
    {
      label->setCheckState( Qt::Checked );
      attributeCombo->setEnabled( true );
    }
    else
    {
      label->setCheckState( Qt::Unchecked );
      attributeCombo->setEnabled( false );
    }

    /* This is more for debugging than for end-user functionality. */
    if( !success )
    {
      showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
    }

    ui->tableWidget->setCellWidget( i, COMBOCOLUMN, attributeCombo );
  }

  updateCheckStates( item );

  ui->tableWidget->horizontalHeader()->setResizeMode( LABELCOLUMN, QHeaderView::Stretch );
  ui->tableWidget->horizontalHeader()->setResizeMode( COMBOCOLUMN, QHeaderView::Stretch );
  ui->tableWidget->horizontalHeader()->setResizeMode( INCRCOLUMN,  QHeaderView::Fixed );
  m_treeItemActivated = false;
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::attributeChanged( QTableWidgetItem *item )
{
  if( !m_treeItemActivated )
  {
    QTreeWidgetItem *treeItem = ui->treeWidget->currentItem();
    GCDomElementInfo *info = const_cast< GCDomElementInfo* >( m_elementInfo.value( treeItem ) );

    GCComboBox *attributeValueCombo = dynamic_cast< GCComboBox* >( ui->tableWidget->cellWidget( item->row(), 1 ) );
    QDomElement currentElement = m_treeItemNodes.value( treeItem );

    if( item->checkState() == Qt::Checked )
    {
      currentElement.setAttribute( item->text(), attributeValueCombo->currentText() );
      attributeValueCombo->setEnabled( true );
      info->includeAttribute( item->text() );
    }
    else
    {
      currentElement.removeAttribute( item->text() );
      attributeValueCombo->setEnabled( false );
      info->excludeAttribute( item->text() );
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::attributeValueChanged()
{
  /* Update the element's attribute inclusions, values and value increment flags. */
  QString elementName = ui->treeWidget->currentItem()->text( LABELCOLUMN );

  /* There will only ever be one element with this name the way this class has been implemented. */
  QDomElement element = m_domDoc.elementsByTagName( elementName ).at( 0 ).toElement();

  QDomNamedNodeMap attributes = element.attributes();

  for( int i = 0; i < attributes.size(); ++i )
  {
    /* First column. */
    QCheckBox *checkBox = dynamic_cast< QCheckBox* >( ui->tableWidget->cellWidget( i, INCRCOLUMN ) );

    /* Second column. */
    QString attributeName = ui->tableWidget->item( i, LABELCOLUMN )->text();

    /* Third column. */
    GCComboBox *comboBox = dynamic_cast< GCComboBox* >( ui->tableWidget->cellWidget( i, COMBOCOLUMN ) );
    QString attributeValue = comboBox->currentText();
    m_originalValues.insert( elementName + attributeName, attributeValue.trimmed() );

    element.setAttribute( attributeName, attributeValue );
    m_attributes.insert( elementName + attributeName, checkBox->isChecked() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::addSnippet()
{
  /* Create a duplicate DOM doc so that we do not remove the elements in their
    entirety...when the user changes the snippet structure, the original DOM must
    be accessible (the DOM containing all known elements). */
  QDomDocument doc = m_domDoc.cloneNode().toDocument();
  QStringList elementNames;

  foreach( QTreeWidgetItem *item, m_treeItemNodes.keys() )
  {
    if( m_elementInfo.value( item )->elementExcluded() )
    {
      /* There will only ever be one element with this name the way this class has been implemented. */
      QDomNode parent = doc.elementsByTagName( m_treeItemNodes.value( item ).tagName() ).at( 0 ).parentNode();
      parent.removeChild( doc.elementsByTagName( m_treeItemNodes.value( item ).tagName() ).at( 0 ) );
    }
    else
    {
      elementNames.append( m_treeItemNodes.value( item ).tagName() );
    }
  }

  /* Note to future self - if this looks weird, it's because it is.  QDomElement's habit of shallow
    copying everything means it's pretty darn difficult getting hold of a tree widget item from the
    m_treeItemNodes map.  This way, although clunkier, works significantly better. Furthermore, the
    reason it works is because there will only be one element of each type...if that wasn't the case
    sticking them into a hash would have been a mess. */
  QHash< QString, QTreeWidgetItem* > treeItems;

  QTreeWidgetItemIterator itemIterator( ui->treeWidget );

  while( *itemIterator )
  {
    if( elementNames.contains( m_treeItemNodes.value( *itemIterator ).tagName() ) )
    {
      treeItems.insert( m_treeItemNodes.value( *itemIterator ).tagName(), ( *itemIterator ) );
    }

    ++itemIterator;
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

      /* Required so that we do not end up manipulating the map itself (each time an attribute is removed,
        the loop will be affected since the map's size changes). */
      QList< QString > attributesToRemove;

      for( int k = 0; k < attributes.size(); ++k )
      {
        QDomAttr attr = attributes.item( k ).toAttr();
        QString attributeValue = m_originalValues.value( elementName + attr.name() );

        if( !m_elementInfo.value( treeItems.value( elementName ) )->includedAttributes().contains( attr.name() ) )
        {
          attributesToRemove << attr.name() ;
        }
        else
        {
          if( m_attributes.value( elementName + attr.name() ) )
          {
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
              there is to it (it's not our responsibility to check that someone isn't incrementing
              "false", e.g.). */
              attributeValue += QString( "%1" ).arg( i );
            }

            element.setAttribute( attr.name(), attributeValue );
          }
        }
      }

      foreach( QString attribute, attributesToRemove )
      {
        element.removeAttribute( attribute );
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
  item->setText( 0, elementName );
  item->setCheckState( 0, Qt::Checked );
  ui->treeWidget->invisibleRootItem()->addChild( item );  // takes ownership

  processNextElement( elementName, item, m_domDoc );

  ui->treeWidget->expandAll();
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::processNextElement( const QString &elementName, QTreeWidgetItem *parent, QDomNode parentNode )
{
  bool success( false );

  QDomElement element = m_domDoc.createElement( elementName );

  /* This looks weird, but remember that the "parent" tree widget item is on the same level
    as the QDomElement. */
  m_treeItemNodes.insert( parent, element );
  m_elementInfo.insert( parent, new GCDomElementInfo( element ) );

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
      item->setText( 0, child );
      item->setCheckState( 0, Qt::Checked );
      parent->addChild( item );  // takes ownership

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

void GCSnippetsForm::updateCheckStates( QTreeWidgetItem *item )
{
  /* Checking or unchecking an item must recursively update its children as well. */
  if( item->checkState( 0 ) == Qt::Checked )
  {
    const_cast< GCDomElementInfo* >( m_elementInfo.value( item ) )->setExcludeElement( false );

    /* When a low-level child is activated, we need to also update its parent tree all the way
      up to the root element since including a child automatically implies that the parent
      element is included.  By the time we reach this point, all the element's children have
      been updated so we can now set the flag to prevent its siblings from being reactivated
      when we set its parent's check state. */
    QTreeWidgetItem *parent = item->parent();

    while( parent && parent->checkState( 0 ) != Qt::Checked )
    {
      parent->setCheckState( 0, Qt::Checked );
      parent = parent->parent();
    }
  }
  else
  {
    const_cast< GCDomElementInfo* >( m_elementInfo.value( item ) )->setExcludeElement( true );
  }

  for( int i = 0; i < item->childCount(); ++i )
  {
    item->child( i )->setCheckState( 0, item->checkState( 0 ) );
    updateCheckStates( item->child( i ) );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCSnippetsForm::deleteElementInfo()
{
  foreach( GCDomElementInfo *info, m_elementInfo.values() )
  {
    delete info;
    info = NULL;
  }

  m_elementInfo.clear();
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
