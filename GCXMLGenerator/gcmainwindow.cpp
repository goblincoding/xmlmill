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

#include "gcmainwindow.h"
#include "ui_gcmainwindow.h"
#include "db/gcdatabaseinterface.h"
#include "xml/xmlsyntaxhighlighter.h"
#include "forms/gcadditemsform.h"
#include "forms/gcremoveitemsform.h"
#include "forms/gchelpdialog.h"
#include "forms/gcsearchform.h"
#include "forms/gcsnippetsform.h"
#include "utils/gccombobox.h"
#include "utils/gcdbsessionmanager.h"
#include "utils/gcmessagespace.h"
#include "utils/gcdomelementinfo.h"

#include <QDesktopServices>
#include <QSignalMapper>
#include <QDomDocument>
#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QTextCursor>
#include <QComboBox>
#include <QTimer>
#include <QModelIndex>
#include <QLabel>
#include <QUrl>
#include <QCloseEvent>

/*--------------------------------------------------------------------------------------*/

const QString EMPTY( "---" );
const qint64  DOMWARNING( 262144 );  // 0.25MB or ~7 500 lines
const qint64  DOMLIMIT  ( 524288 );  // 0.5MB  or ~15 000 lines

/*--------------------------------- MEMBER FUNCTIONS ----------------------------------*/

GCMainWindow::GCMainWindow( QWidget *parent ) :
  QMainWindow           ( parent ),
  ui                    ( new Ui::GCMainWindow ),
  m_dbSessionManager    ( new GCDBSessionManager( this ) ),
  m_signalMapper        ( new QSignalMapper( this ) ),
  m_domDoc              ( NULL ),
  m_currentCombo        ( NULL ),
  m_saveTimer           ( NULL ),
  m_activeSessionLabel  ( NULL ),
  m_currentXMLFileName  ( "" ),
  m_activeAttributeName ( "" ),
  m_wasTreeItemActivated( false ),
  m_newAttributeAdded   ( false ),
  m_busyImporting       ( false ),
  m_DOMTooLarge         ( false ),
  m_elementInfo         (),
  m_treeItemNodes       (),
  m_comboBoxes          ()
{
  ui->setupUi( this );

  /* XML File related. */
  connect( ui->actionNew, SIGNAL( triggered() ), this, SLOT( newXMLFile() ) );
  connect( ui->actionOpen, SIGNAL( triggered() ), this, SLOT( openXMLFile() ) );
  connect( ui->actionSave, SIGNAL( triggered() ), this, SLOT( saveXMLFile() ) );
  connect( ui->actionSaveAs, SIGNAL( triggered() ), this, SLOT( saveXMLFileAs() ) );
  connect( ui->actionCloseFile, SIGNAL( triggered() ), this, SLOT( resetDOM() ) );

  /* Build/Edit XML. */
  connect( ui->deleteElementButton, SIGNAL( clicked() ), this, SLOT( deleteElementFromDocument() ) );
  connect( ui->addChildElementButton, SIGNAL( clicked() ), this, SLOT( addElementToDocument() ) );
  connect( ui->addSnippetButton, SIGNAL( clicked() ), this, SLOT( addSnippetToDocument() ) );
  connect( ui->textSaveButton, SIGNAL( clicked() ), this, SLOT( saveDirectEdit() ) );
  connect( ui->textRevertButton, SIGNAL( clicked() ), this, SLOT( revertDirectEdit() ) );
  connect( ui->domEditHelpButton, SIGNAL( clicked() ), this, SLOT( showDOMEditHelp() ) );

  /* Various other actions. */
  connect( ui->actionExit, SIGNAL( triggered() ), this, SLOT( close() ) );
  connect( ui->actionFind, SIGNAL( triggered() ), this, SLOT( searchDocument() ) );
  connect( ui->actionForgetPreferences, SIGNAL( triggered() ), this, SLOT( forgetMessagePreferences() ) );
  connect( ui->actionHelpContents, SIGNAL( triggered() ), this, SLOT( showMainHelp() ) );
  connect( ui->actionVisitOfficialSite, SIGNAL( triggered() ), this, SLOT( goToSite() ) );
  connect( ui->expandAllCheckBox, SIGNAL( clicked( bool ) ), this, SLOT( collapseOrExpandTreeWidget( bool ) ) );

  /* Everything tree widget related. */
  connect( ui->treeWidget, SIGNAL( itemClicked ( QTreeWidgetItem*, int ) ), this, SLOT( elementSelected( QTreeWidgetItem*, int ) ) );
  connect( ui->treeWidget, SIGNAL( itemActivated( QTreeWidgetItem*, int ) ), this, SLOT( elementSelected( QTreeWidgetItem*, int ) ) );
  connect( ui->treeWidget, SIGNAL( itemChanged( QTreeWidgetItem*, int ) ), this, SLOT( elementChanged( QTreeWidgetItem*, int ) ) );

  /* Everything table widget related. */
  connect( ui->tableWidget, SIGNAL( itemChanged( QTableWidgetItem* ) ), this, SLOT( attributeChanged( QTableWidgetItem* ) ) );
  connect( ui->tableWidget, SIGNAL( itemClicked( QTableWidgetItem* ) ), this, SLOT( attributeSelected( QTableWidgetItem* ) ) );

  /* Database related. */
  connect( ui->actionAddItems, SIGNAL( triggered() ), this, SLOT( showAddItemsForm() ) );
  connect( ui->actionRemoveItems, SIGNAL( triggered() ), this, SLOT( showRemoveItemsForm() ) );
  connect( ui->actionImportXMLToDatabase, SIGNAL( triggered() ), this, SLOT( importXMLToDatabase() ) );
  connect( ui->actionSwitchSessionDatabase, SIGNAL( triggered() ), this, SLOT( switchActiveDatabase() ) );
  connect( ui->actionAddNewDatabase, SIGNAL( triggered() ), this, SLOT( addNewDatabase() ) );
  connect( ui->actionAddExistingDatabase, SIGNAL( triggered() ), this, SLOT( addExistingDatabase() ) );
  connect( ui->actionRemoveDatabase, SIGNAL( triggered() ), this, SLOT( removeDatabase() ) );
  connect( m_dbSessionManager, SIGNAL( reset() ), this, SLOT( resetDOM() ) );
  connect( m_dbSessionManager, SIGNAL( activeDatabaseChanged( QString ) ), this, SLOT( activeDatabaseChanged( QString ) ) );

  /* Initialise the database interface and retrieve the list of database names (this will
    include the path references to the ".db" files). */
  if( !GCDataBaseInterface::instance()->initialised() )
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
    this->close();
  }

  m_domDoc = new QDomDocument;

  /* If the interface was successfully initialised, prompt the user to choose a database
    connection for this session. */
  m_dbSessionManager->showKnownDBForm( GCKnownDBForm::ShowAll );

  /* Everything happens automagically and the text edit takes ownership. */
  XmlSyntaxHighlighter *highLighter = new XmlSyntaxHighlighter( ui->dockWidgetTextEdit );
  Q_UNUSED( highLighter );

  connect( m_signalMapper, SIGNAL( mapped( QWidget* ) ), this, SLOT( setCurrentComboBox( QWidget* ) ) );
}

/*--------------------------------------------------------------------------------------*/

GCMainWindow::~GCMainWindow()
{
  clearElementInfo();
  delete m_domDoc;
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::closeEvent( QCloseEvent *event )
{
  if( !m_domDoc->documentElement().isNull() )
  {
    QMessageBox::StandardButtons accept = QMessageBox::question( this,
                                                                 "Save File?",
                                                                 "Save changes before closing?",
                                                                 QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                                                 QMessageBox::Yes );

    if( accept == QMessageBox::Yes )
    {
      saveXMLFile();
    }
    else if( accept == QMessageBox::Cancel )
    {
      event->ignore();
      return;
    }
  }

  QMainWindow::closeEvent( event );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::elementChanged( QTreeWidgetItem *item, int column )
{
  /* This check is probably redundant, but rather safe than sorry... */
  if( m_treeItemNodes.contains( item ) )
  {
    QString elementName  = item->text( column );
    QString previousName = m_treeItemNodes.value( item ).toElement().tagName();

    /* Watch out for empty strings. */
    if( elementName.isEmpty() )
    {
      showErrorMessageBox( "Sorry, but we don't know what to do with empty names..." );
      item->setText( column, previousName );
    }
    else
    {
      /* If the element name didn't change, do nothing. */
      if( elementName != previousName )
      {
        /* Update the element names in our active DOM doc (since "m_treeItemNodes"
          contains shallow copied QDomElements, the change will automatically
          be applied to the map's nodes as well) and the tree widget. */
        QDomNodeList elementList = m_domDoc->elementsByTagName( previousName );

        for( int i = 0; i < elementList.count(); ++i )
        {
          QDomElement element( elementList.at( i ).toElement() );
          element.setTagName( elementName );
          const_cast< QTreeWidgetItem* >( m_treeItemNodes.key( element ) )->setText( column, elementName );
        }

        /* The name change may introduce a new element name to the DB, we can safely call
          "addElement" below as it doesn't do anything if the element already exists in the database. */
        bool success( false );
        QStringList attributes = GCDataBaseInterface::instance()->attributes( previousName, success );

        if( !GCDataBaseInterface::instance()->addElement( elementName,
                                                          GCDataBaseInterface::instance()->children( previousName, success ),
                                                          attributes ) )
        {
          showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
        }

        /* Since the user only really changed the element name, we also want the "new" element to be automatically
          associated with the attributes connected to the previous name (as well as the attributes' known values). */
        foreach( QString attribute, attributes )
        {
          if( !GCDataBaseInterface::instance()->updateAttributeValues( elementName,
                                                                       attribute,
                                                                       GCDataBaseInterface::instance()->attributeValues( previousName, attribute, success ) ) )
          {
            showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
          }
        }

        if( !success )
        {
          showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
        }

        setTextEditContent( m_treeItemNodes.value( item ).toElement() );
      }
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::elementSelected( QTreeWidgetItem *item, int column )
{
  Q_UNUSED( column );

  /* Because the table widget is re-populated with the attribute names and
    values associated with the activated tree widget item, this flag is set
    to prevent the functionality in "attributeChanged" (which is triggered
    by the population of the table widget) from being executed. */
  m_wasTreeItemActivated = true;

  resetTableWidget();

  /* Get only the attributes currently assigned to the element
    corresponding to the activated item (and the lists of associated
    values for these attributes) and populate our table widget. */
  bool success( false );
  QDomElement element = m_treeItemNodes.value( item );
  QString elementName = element.tagName();
  QStringList attributeNames = GCDataBaseInterface::instance()->attributes( elementName, success );

  /* This is more for debugging than for end-user functionality. */
  if( !success )
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
  }

  /* Add all the known attribute names to the cells in the first column
    of the table widget, create and populate combo boxes with the values
    associated with the attributes in question and insert the combo boxes
    into the second column of the table widget. Finally we insert an
    "empty" row so that the user may add additional attributes and values
    to the current element. */
  for( int i = 0; i < attributeNames.count(); ++i )
  {
    QTableWidgetItem *label = new QTableWidgetItem( attributeNames.at( i ) );
    label->setFlags( label->flags() | Qt::ItemIsUserCheckable );

    ui->tableWidget->setRowCount( i + 1 );
    ui->tableWidget->setItem( i, 0, label );

    GCComboBox *attributeCombo = new GCComboBox;
    attributeCombo->addItems( GCDataBaseInterface::instance()->attributeValues( elementName, attributeNames.at( i ), success ) );
    attributeCombo->setEditable( true );

    /* This is more for debugging than for end-user functionality. */
    if( !success )
    {
      showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
    }

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

    /* If we are still in the process of building the document, the attribute value will
      be empty since it has never been set before.  For this particular case, calling
      "findText" will result in a null pointer exception being thrown so we need to
      cater for this possibility here. */
    QString attributeValue = element.attribute( attributeNames.at( i ) );

    if( !attributeValue.isEmpty() )
    {
      attributeCombo->setCurrentIndex( attributeCombo->findText( attributeValue ) );
    }
    else
    {
      attributeCombo->setCurrentIndex( 0 );
    }

    /* Attempting the connection before we've set the current index above causes the
      "attributeValueChanged" slot to be called prematurely, resulting in a segmentation
      fault due to value conflicts/missing values (in short, we can't do the connect
      before we've set the current index). */
    connect( attributeCombo, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( attributeValueChanged( QString ) ) );

    ui->tableWidget->setCellWidget( i, 1, attributeCombo );
    m_comboBoxes.insert( attributeCombo, i );

    /* This will point the current combo box member to the combo that's been activated
      in the table widget (used in "attributeValueChanged" to obtain the row number the
      combo box appears in in the table widget, etc, etc). */
    connect( attributeCombo, SIGNAL( activated( int ) ), m_signalMapper, SLOT( map() ) );
    m_signalMapper->setMapping( attributeCombo, attributeCombo );
  }

  /* Add the "empty" row as described above. */
  insertEmptyTableRow();

  /* Populate the "add child element" combo box with the known first level children of the
    current highlighted element (highlighted in the tree widget, of course). */
  ui->addElementComboBox->clear();
  ui->addElementComboBox->addItems( GCDataBaseInterface::instance()->children( elementName, success ) );

  /* The following will be used to allow the user to add an element of the current type to
    its parent (this should improve the user experience as they do not have to explicitly
    navigate back up to a parent when adding multiple items of the same type).  We also don't
    want the user to add a document root element to itself by accident. */
  if( elementName != m_domDoc->documentElement().tagName() )
  {
    ui->addElementComboBox->addItem( QString( "<%1>" ).arg( elementName) );
  }

  toggleAddElementWidgets();

  /* This is more for debugging than for end-user functionality. */
  if( !success )
  {
    showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
  }

  ui->dockWidgetTextEdit->moveCursor( QTextCursor::Start );
  setTextEditContent( element );

  /* The user must not be allowed to add an entire document as a "snippet". */
  if( ui->addElementComboBox->currentText() == m_domDoc->documentElement().tagName() )
  {
    ui->addSnippetButton->setEnabled( false );
  }
  else
  {
    ui->addSnippetButton->setEnabled( true );
  }

  /* Unset flag. */
  m_wasTreeItemActivated = false;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::attributeChanged( QTableWidgetItem *item )
{
  /* Don't execute the logic if a tree widget item's activation is triggering
    a re-population of the table widget (which results in this slot being called). */
  if( !m_wasTreeItemActivated && !m_newAttributeAdded )
  {
    /* All attribute name changes will be assumed to be additions, removing an attribute
      with a specific name has to be done explicitly. */
    QTreeWidgetItem *treeItem = ui->treeWidget->currentItem();
    GCComboBox *attributeValueCombo = dynamic_cast< GCComboBox* >( ui->tableWidget->cellWidget( item->row(), 1 ) );
    QDomElement currentElement = m_treeItemNodes.value( treeItem );

    /* See if an existing attribute's name changed or if this slot was called due to
      an attribute's exclude/include state toggle. */
    if( item->text() != m_activeAttributeName )
    {
      /* Add the new attribute's name to the current element's list of associated attributes. */
      if( GCDataBaseInterface::instance()->updateElementAttributes( currentElement.tagName(), QStringList( item->text() ) ) )
      {
        /* If this is an entirely new attribute (i.e. not a name change), we won't have any known
          associated values to add to it just yet. */
        if( m_activeAttributeName != EMPTY )
        {
          currentElement.removeAttribute( m_activeAttributeName );

          /* Retrieve the list of values associated with the previous name and insert it against the new name. */
          bool success;
          QStringList attributeValues = GCDataBaseInterface::instance()->attributeValues( currentElement.tagName(),
                                                                                          m_activeAttributeName,
                                                                                          success );

          if( success )
          {
            if( !GCDataBaseInterface::instance()->updateAttributeValues( currentElement.tagName(), item->text(), attributeValues ) )
            {
              showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
            }
          }
          else
          {
            showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
          }
        }
        else
        {
          m_newAttributeAdded = true;
          item->setFlags( item->flags() | Qt::ItemIsUserCheckable );
          item->setCheckState( Qt::Checked );

          /* If this is an entirely new attribute, insert another new "empty" row so that the user may add
            even more attributes if he/she wishes to do so. */
          insertEmptyTableRow();

          m_newAttributeAdded = false;
        }
      }
      else
      {
        showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
      }
    }

    /* The attribute name may have changed despite it being inactive so let's see what the user
      intends to do with this attribute for this particular document. */
    GCDomElementInfo *info = const_cast< GCDomElementInfo* >( m_elementInfo.value( treeItem ) );

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

    m_activeAttributeName = item->text();
    setTextEditContent( currentElement );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::attributeSelected( QTableWidgetItem *item )
{
  m_activeAttributeName = item->text();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setCurrentComboBox( QWidget *combo )
{
  m_currentCombo = combo;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::attributeValueChanged( const QString &value )
{
  /* Don't execute the logic if a tree widget item's activation is triggering
    a re-population of the table widget (which results in this slot being called). */
  if( !m_wasTreeItemActivated )
  {
    QTreeWidgetItem *currentItem = ui->treeWidget->currentItem();
    QDomElement currentElement = m_treeItemNodes.value( currentItem );

    /* The current attribute will be displayed in the first column (next to the
    combo box which will be the actual current item). */
    QString currentAttributeName = ui->tableWidget->item( m_comboBoxes.value( m_currentCombo ), 0 )->text();

    /* If the user sets the attribute value to EMPTY, the attribute is removed from the current document. */
    if( value == EMPTY )
    {
      currentElement.removeAttribute( currentAttributeName );
    }
    else
    {
      currentElement.setAttribute( currentAttributeName, value );

      /* If we don't know about this value, we need to add it to the DB. */
      bool success( false );
      QStringList attributeValues = GCDataBaseInterface::instance()->attributeValues( currentElement.tagName(), currentAttributeName, success );

      if( success )
      {
        if( !attributeValues.contains( value ) )
        {
          if( !GCDataBaseInterface::instance()->updateAttributeValues( currentElement.tagName(),
                                                                       currentAttributeName,
                                                                       QStringList( value ) ) )
          {
            showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
          }
        }
      }
      else
      {
        showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
      }
    }

    setTextEditContent( currentElement );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::newXMLFile()
{
  resetDOM();
  m_currentXMLFileName = "";
  ui->actionCloseFile->setEnabled( true );
  ui->actionSaveAs->setEnabled   ( true );
  ui->actionSave->setEnabled     ( true );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::openXMLFile()
{
  /* Can't open a file if there is no DB profile to describe it. */
  if( !GCDataBaseInterface::instance()->hasActiveSession() )
  {
    QString errMsg( "No active profile set, please set one for this session." );
    showErrorMessageBox( errMsg );
    m_dbSessionManager->showKnownDBForm( GCKnownDBForm::ShowAll );
    return;
  }

  QString fileName = QFileDialog::getOpenFileName( this, "Open File", QDir::homePath(), "XML Files (*.*)" );

  /* If the user clicked "OK", continue (a cancellation will result in an empty file name). */
  if( !fileName.isEmpty() )
  {
    m_currentXMLFileName = fileName;
    QFile file( m_currentXMLFileName );

    if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      QString errorMsg = QString( "Failed to open file \"%1\": [%2]" )
          .arg( fileName )
          .arg( file.errorString() );
      showErrorMessageBox( errorMsg );
      m_currentXMLFileName = "";
      return;
    }

    /* Reset the DOM only after we've successfully opened the file. */
    resetDOM();

    QTextStream inStream( &file );
    QString fileContent( inStream.readAll() );
    qint64 fileSize = file.size();
    file.close();

    showLargeFileWarnings( fileSize );

    QString xmlErr( "" );
    int     line  ( -1 );
    int     col   ( -1 );

    if( !m_domDoc->setContent( fileContent, &xmlErr, &line, &col ) )
    {
      QString errorMsg = QString( "XML is broken - Error [%1], line [%2], column [%3]" )
          .arg( xmlErr )
          .arg( line )
          .arg( col );
      showErrorMessageBox( errorMsg );
      resetDOM();
      m_currentXMLFileName = "";
      return;
    }

    /* If the user is opening an XML file of a kind that isn't supported by the current active session,
      we need to warn him/her of this fact and provide them with a couple of options (depending on which
      privileges the current user mode has). */
    if( !GCDataBaseInterface::instance()->knownRootElements().contains( m_domDoc->documentElement().tagName() ) )
    {
      /* If we're not already busy importing an XML file, check if the user maybe wants to do so. */
      if( !m_busyImporting )
      {
        bool accepted = GCMessageSpace::userAccepted( "QueryImportXML",
                                                      "Import XML?",
                                                      "The current active profile has no knowledge of the"
                                                      "specific XML style of the document you are trying to open.\n\n"
                                                      "Would you like to import the XML document to the active profile?",
                                                      GCMessageDialog::YesNo,
                                                      GCMessageDialog::No,
                                                      GCMessageDialog::Question );

        if( accepted )
        {
          importXMLToDatabase();
        }
        else
        {
          /* If the user decided to rather abort the file opening process, reset everything. */
          resetDOM();
          m_currentXMLFileName = "";
        }
      }
      else
      {
        /* If we're already busy importing, it means the user explicitly requested
        an XML import (via the "Database" menu), didn't have a current document active
        and confirmed that he/she wanted to open an XML file to import.  Furthermore,
        there is no risk of an endless loop since the DOM document will have been
        populated by the time we get to this point, which will ensure that only the
        first part of the following function's logic will be executed..."openXMLFile"
        won't be called again. */
        importXMLToDatabase();
      }
    }
    else
    {
      /* If the user is currently busy importing XML, batch process the new file. */
      if( m_busyImporting )
      {
        importXMLToDatabase();
      }

      /* If the user selected a database that knows of this particular XML profile,
      simply process the document. */
      processDOMDoc();
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::saveXMLFile()
{
  if( m_currentXMLFileName.isEmpty() )
  {
    saveXMLFileAs();
  }
  else
  {
    QFile file( m_currentXMLFileName );

    if( !file.open( QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text ) )
    {
      QString errMsg = QString( "Failed to save file \"%1\": [%2]." )
          .arg( m_currentXMLFileName )
          .arg( file.errorString() );
      showErrorMessageBox( errMsg );
    }
    else
    {
      QTextStream outStream( &file );
      outStream << m_domDoc->toString( 2 );
      file.close();

      startSaveTimer();
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::saveXMLFileAs()
{
  QString file = QFileDialog::getSaveFileName( this, "Save As", QDir::homePath(), "XML Files (*.*)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    m_currentXMLFileName = file;
    saveXMLFile();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addNewDatabase() const
{
  /* If we have an active DOM document, we need to pass the name of the root
    element through to the DB session manager which uses it to determine whether
    or not a user is on the verge of messing something up... */
  if( m_domDoc->documentElement().isNull() )
  {
    m_dbSessionManager->addNewDatabase();
  }
  else
  {
    m_dbSessionManager->addNewDatabase( m_domDoc->documentElement().tagName() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addExistingDatabase() const
{
  /* If we have an active DOM document, we need to pass the name of the root
    element through to the DB session manager which uses it to determine whether
    or not a user is on the verge of messing something up... */
  if( m_domDoc->documentElement().isNull() )
  {
    m_dbSessionManager->addExistingDatabase();
  }
  else
  {
    m_dbSessionManager->addExistingDatabase( m_domDoc->documentElement().tagName() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::removeDatabase() const
{
  /* If we have an active DOM document, we need to pass the name of the root
    element through to the DB session manager which uses it to determine whether
    or not a user is on the verge of messing something up... */
  if( m_domDoc->documentElement().isNull() )
  {
    m_dbSessionManager->removeDatabase();
  }
  else
  {
    m_dbSessionManager->removeDatabase( m_domDoc->documentElement().tagName() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::switchActiveDatabase() const
{
  /* If we have an active DOM document, we need to pass the name of the root
    element through to the DB session manager which uses it to determine whether
    or not a user is on the verge of messing something up... */
  if( m_domDoc->documentElement().isNull() )
  {
    m_dbSessionManager->switchActiveDatabase();
  }
  else
  {
    m_dbSessionManager->switchActiveDatabase( m_domDoc->documentElement().tagName() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::importXMLToDatabase()
{
  /* Set this flag in case the user doesn't have an active DOM document to import
    and wishes to open an XML file (the flag's status is used in "openXMLFile"). */
  m_busyImporting = true;

  if (!m_domDoc->documentElement().isNull() )
  {
    /* Update the DB with the contents of the current DOM document. */
    if( !GCDataBaseInterface::instance()->batchProcessDOMDocument( m_domDoc ) )
    {
      showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
    }
    else
    {
      processDOMDoc();
    }
  }
  else
  {
    bool accepted = GCMessageSpace::userAccepted( "QueryImportXMLFromFile",
                                                  "No active document",
                                                  "There is no document currently active, "
                                                  "would you like to import the XML from file?",
                                                  GCMessageDialog::YesNo,
                                                  GCMessageDialog::Yes,
                                                  GCMessageDialog::Question );
    if( accepted )
    {
      openXMLFile();
    }
  }

  /* Unset flag. */
  m_busyImporting = false;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::deleteElementFromDocument()
{
  QTreeWidgetItem *currentItem = ui->treeWidget->currentItem();
  QDomElement currentElement = m_treeItemNodes.value( currentItem );

  /* If all we have is a document root element, reset everything. */
  if( currentElement == m_domDoc->documentElement() )
  {
    resetDOM();
  }
  else
  {
    /* Remove the element from the DOM first. */
    QDomNode parentNode = currentElement.parentNode();
    parentNode.removeChild( currentElement );

    /* Now we can whack it from the tree widget and map. */
    m_treeItemNodes.remove( currentItem );

    GCDomElementInfo *info = m_elementInfo.value( currentItem );

    if( info )
    {
      delete info;
      info = NULL;
    }

    m_elementInfo.remove( currentItem );

    QTreeWidgetItem *parentItem = currentItem->parent();
    parentItem->removeChild( currentItem );

    /* Repopulate the table widget with values from whichever
      element is highlighted after the removal. */
    elementSelected( currentItem, 0 );

    setTextEditContent( parentNode.toElement() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addElementToDocument()
{
  QString newElementName = ui->addElementComboBox->currentText();

  /* If we already have mapped element nodes, add the new item to whichever
    parent item is currently highlighted in the tree widget. */
  QTreeWidgetItem *item( NULL );

  if( !m_treeItemNodes.isEmpty() )
  {
    item = ui->treeWidget->currentItem();
  }

  /* If the user selected the <element> option, we add a new sibling element
    of the same name as the current element to the current element's parent. */
  if( newElementName.contains( "<" ) )
  {
    newElementName = newElementName.remove( QRegExp( "<|>" ) );
    item = item->parent();
  }

  addElementToDocument( newElementName, item );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addSnippetToDocument()
{
  QString elementName = ui->addElementComboBox->currentText();

  /* Check if we're inserting snippets as children, or as siblings. */
  if( elementName.contains( QRegExp( "<|>" ) ) )
  {
    GCSnippetsForm *dialog = new GCSnippetsForm( elementName.remove( QRegExp( "<|>" ) ),
                                                 m_treeItemNodes.value( ui->treeWidget->currentItem()->parent() ),
                                                 this );
    connect( dialog, SIGNAL( snippetAdded() ), this, SLOT( insertSnippet() ) );
    dialog->exec();
  }
  else
  {
    GCSnippetsForm *dialog = new GCSnippetsForm( elementName,
                                                 m_treeItemNodes.value( ui->treeWidget->currentItem() ),
                                                 this );
    connect( dialog, SIGNAL( snippetAdded() ), this, SLOT( insertSnippet() ) );
    dialog->exec();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::insertSnippet()
{
  /* Since we directly manipulated the existing DOM through the "insert snippets" form,
    all we have to do here is to update the tree and the text edit. */
  processDOMDoc();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::resetDOM()
{
  m_domDoc->clear();
  m_treeItemNodes.clear();
  ui->treeWidget->clear();
  ui->dockWidgetTextEdit->clear();
  resetTableWidget();
  clearElementInfo();

  ui->addElementComboBox->clear();
  ui->addElementComboBox->addItems( GCDataBaseInterface::instance()->knownRootElements() );
  toggleAddElementWidgets();

  ui->addSnippetButton->setEnabled( false );

  m_currentCombo = NULL;
  m_activeAttributeName = "";
  m_DOMTooLarge = false;

  /* The timer will be reactivated as soon as work starts again on a legitimate
    document and the user saves it for the first time. */
  if( m_saveTimer )
  {
    m_saveTimer->stop();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showRemoveItemsForm()
{
  if( !GCDataBaseInterface::instance()->hasActiveSession() )
  {
    showErrorMessageBox( "No session currently active" );
    return;
  }

  if( !m_domDoc->documentElement().isNull() )
  {
    showErrorMessageBox( "\"Save\" and \"Close\" the current document before continuing.");
    return;
  }

  /* Delete on close flag set. */
  GCRemoveItemsForm *dialog = new GCRemoveItemsForm( this );
  dialog->exec();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showAddItemsForm()
{
  /* Check if there is a previously saved user preference for this action (we
    don't want to remember a \"Cancel\" option for this particular situation). */
  bool accepted = GCMessageSpace::userAccepted( "WarnNewElementsAsFirstLevelChildren",
                                                "Careful!",
                                                "All the new elements you add will be added as first level "
                                                "children of the element highlighted in the tree view (in "
                                                "other words it will become a sibling to the elements currently "
                                                "in the dropdown menu).\n\n"
                                                "If this is not what you intended, I suggest you \"Cancel\".",
                                                GCMessageDialog::OKCancel,
                                                GCMessageDialog::OK,
                                                GCMessageDialog::Warning,
                                                false );

  if( accepted )
  {
    GCAddItemsForm *form = new GCAddItemsForm( this );
    form->exec();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::revertDirectEdit()
{
  setTextEditContent( QDomElement() );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::saveDirectEdit()
{
  QString xmlErr( "" );
  int     line  ( -1 );
  int     col   ( -1 );

  /* The reason for creating a temporary document is so that we do not mess with the contents
    of the tree item node map.  Breaking the content of the DOM member has an adverse effect
    on the map and the reason is as follows - the mapped nodes are shallow copies of the nodes
    in the DOM, when the DOM gets broken, the map isn't updated to reflect the changes which means
    that, when the map is destructed, it attempts to call destructors on nodes that no longer exist
    and bad things happen. */
  QDomDocument doc;
  if( !doc.setContent( ui->dockWidgetTextEdit->toPlainText(), &xmlErr, &line, &col ) )
  {
    QString errorMsg = QString( "XML is broken - Error [%1], line [%2], column [%3]." )
        .arg( xmlErr )
        .arg( line )
        .arg( col );
    showErrorMessageBox( errorMsg );

    /* Unfortunate that the line number returned by the DOM doc doesn't match up with what's
      visible in the QTextEdit.  It seems as if it's mostly off by two lines.  For now it's a
      fix, but will have to figure out how to make sure that we highlight the correct lines.
      Ultimately this finds the broken XML and highlights it in red...what a mission... */
    QTextBlock textBlock = ui->dockWidgetTextEdit->document()->findBlockByLineNumber( line - 2 );
    QTextCursor cursor( textBlock );
    cursor.movePosition( QTextCursor::NextWord );
    cursor.movePosition( QTextCursor::EndOfBlock, QTextCursor::KeepAnchor );

    QTextEdit::ExtraSelection highlight;
    highlight.cursor = cursor;
    highlight.format.setBackground( QColor( 220, 150, 220 ) );
    highlight.format.setProperty  ( QTextFormat::FullWidthSelection, true );

    QList< QTextEdit::ExtraSelection > extras;
    extras << highlight;
    ui->dockWidgetTextEdit->setExtraSelections( extras );
    ui->dockWidgetTextEdit->ensureCursorVisible();
  }
  else
  {
    m_domDoc->setContent( ui->dockWidgetTextEdit->toPlainText() );
    importXMLToDatabase();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::elementFound( const QDomElement &element )
{
  QTreeWidgetItem *item = m_treeItemNodes.key( element );
  ui->treeWidget->expandAll();
  ui->treeWidget->setCurrentItem( item );
  elementSelected( item, 0 );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::activeDatabaseChanged( QString dbName )
{
  if( m_domDoc->documentElement().isNull() )
  {
    resetDOM();
  }

  if( !m_activeSessionLabel )
  {
    m_activeSessionLabel = new QLabel( QString( "Active Session Name: %1" ).arg( dbName ) );
    statusBar()->addWidget( m_activeSessionLabel );
  }
  else
  {
    m_activeSessionLabel->setText( QString( "Active Session Name: %1" ).arg( dbName ) );
  }
}


/*--------------------------------------------------------------------------------------*/

void GCMainWindow::collapseOrExpandTreeWidget( bool checked )
{
  if( checked )
  {
    ui->treeWidget->expandAll();
  }
  else
  {
    ui->treeWidget->collapseAll();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::searchDocument()
{
  /* Delete on close flag set. */
  GCSearchForm *form = new GCSearchForm( m_treeItemNodes.values(), m_domDoc->toString(), this );
  connect( form, SIGNAL( foundElement( QDomElement ) ), this, SLOT( elementFound( QDomElement ) ) );
  form->exec();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::forgetMessagePreferences()
{
  GCMessageSpace::forgetAllPreferences();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showDOMEditHelp()
{
  QMessageBox::information( this,
                            "Direct Edits",
                            "Changes to manually edited XML can only be reverted before you hit \"Save\". "
                            "In other words, it isn't an \"undo\" function so please make sure you don't "
                            "save unless you're absolutely sure of your changes." );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showMainHelp()
{
  QFile file( ":/resources/Help.txt" );

  if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
  {
    showErrorMessageBox( QString( "Failed to open \"Help\" file: [%1]" ).arg( file.errorString() ) );
  }
  else
  {
    QTextStream stream( &file );
    QString fileContent = stream.readAll();
    file.close();

    /* Qt::WA_DeleteOnClose flag set...no cleanup required. */
    GCHelpDialog *dialog = new GCHelpDialog( fileContent, this );

    dialog->show();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::goToSite()
{
  QDesktopServices::openUrl( QUrl( "http://goblincoding.com" ) );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::processDOMDoc()
{
  ui->treeWidget->clear(); // also deletes current items
  m_treeItemNodes.clear();
  clearElementInfo();
  resetTableWidget();

  QDomElement root = m_domDoc->documentElement();

  /* Set the document root as the first item in the tree widget. */
  QTreeWidgetItem *item = new QTreeWidgetItem;
  item->setText( 0, root.tagName() );
  item->setFlags( item->flags() | Qt::ItemIsEditable );

  ui->treeWidget->invisibleRootItem()->addChild( item );  // takes ownership
  m_treeItemNodes.insert( item, root );

  m_elementInfo.insert( item, new GCDomElementInfo( root ) );

  /* Now we can recursively stick the rest of the elements into the tree widget. */
  populateTreeWidget( root, item );
  ui->addSnippetButton->setEnabled( true );

  /* Enable file save options. */
  ui->actionCloseFile->setEnabled( true );
  ui->actionSave->setEnabled     ( true );
  ui->actionSaveAs->setEnabled   ( true );

  /* Display the DOM content in the text edit. */
  setTextEditContent( QDomElement() );

  ui->treeWidget->setCurrentItem( item, 0 );
  elementSelected( item, 0 );

  collapseOrExpandTreeWidget( ui->expandAllCheckBox->isChecked() );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::populateTreeWidget( const QDomElement &parentElement, QTreeWidgetItem *parentItem )
{
  QDomElement element = parentElement.firstChildElement();

  while( !element.isNull() )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText( 0, element.tagName() );
    item->setFlags( item->flags() | Qt::ItemIsEditable );

    parentItem->addChild( item );  // takes ownership
    m_treeItemNodes.insert( item, element );

    m_elementInfo.insert( item, new GCDomElementInfo( element ) );

    populateTreeWidget( element, item );
    element = element.nextSiblingElement();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addElementToDocument( const QString &elementName, QTreeWidgetItem *parentItem )
{
  /* There is probably no chance of this ever happening, but defensive programming FTW! */
  if( !elementName.isEmpty() )
  {
    /* Update the tree widget. */
    QTreeWidgetItem *newItem = new QTreeWidgetItem;
    newItem->setText( 0, elementName );
    newItem->setFlags( newItem->flags() | Qt::ItemIsEditable );

    /* Update the current DOM document by creating and adding the new element. */
    QDomElement newElement = m_domDoc->createElement( elementName );

    /* If we already have mapped element nodes, add the new item to whichever
      parent item is currently highlighted in the tree widget. */
    if( parentItem )
    {
      parentItem->addChild( newItem );

      /* Expand the item's parent for convenience. */
      ui->treeWidget->expandItem( parentItem );

      QDomElement parent = m_treeItemNodes.value( parentItem );
      parent.appendChild( newElement );
    }
    else
    {
      /* If the user starts creating a DOM document without having explicitly asked for
      a new file to be created, do it automatically (we can't call "newXMLFile here" since
      it resets the DOM document as well). */
      m_currentXMLFileName = "";
      ui->actionCloseFile->setEnabled( true );
      ui->actionSave->setEnabled     ( true );
      ui->actionSaveAs->setEnabled   ( true );

      /* Since we don't have any existing nodes if we get to this point, we need to initialise
        the tree widget with its first item. */
      ui->treeWidget->invisibleRootItem()->addChild( newItem );  // takes ownership
      m_domDoc->appendChild( newElement );
      ui->addSnippetButton->setEnabled( true );
    }

    /* Keep everything in sync in the maps. */
    m_treeItemNodes.insert( newItem, newElement );    
    m_elementInfo.insert( newItem, new GCDomElementInfo( newElement ) );

    /* Add the known attributes associated with this element. */
    bool success( false );
    QStringList attributes = GCDataBaseInterface::instance()->attributes( elementName, success );

    if( success )
    {
      for( int i = 0; i < attributes.size(); ++i )
      {
        newElement.setAttribute( attributes.at( i ), QString( "" ) );
      }
    }
    else
    {
      showErrorMessageBox( GCDataBaseInterface::instance()->getLastError() );
    }

    setTextEditContent( newElement );
    ui->treeWidget->setCurrentItem( newItem, 0 );
    elementSelected( newItem, 0 );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setStatusBarMessage( const QString &message )
{
  // TODO.
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showErrorMessageBox( const QString &errorMsg )
{
  QMessageBox::critical( this, "Error!", errorMsg );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setTextEditContent( const QDomElement &element )
{
  if( !m_DOMTooLarge )
  {
    ui->dockWidgetTextEdit->setPlainText( m_domDoc->toString( 2 ) );

    if( !element.isNull() )
    {
      ui->dockWidgetTextEdit->find( m_elementInfo.value( m_treeItemNodes.key( element ) )->toString() );
      ui->dockWidgetTextEdit->ensureCursorVisible();
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showLargeFileWarnings( qint64 fileSize )
{
  /* This application isn't optimised for dealing with very large XML files (the entire point is that
    this suite should provide the functionality necessary for the manual manipulation of, e.g. XML config
    files normally set up by hand via copy and paste exercises), if this file is too large to be handled
    comfortably, we need to let the user know and also make sure that we don't try to set the DOM content
    as text in the QTextEdit (QTextEdit is optimised for paragraphs). */
  if( fileSize > DOMWARNING &&
      fileSize < DOMLIMIT )
  {
    /* There is nothing to be acted upon for this particular message so no need
      to check what value the form returns. */ 
    GCMessageSpace::userAccepted( "LargeFileWarning",
                                  "Large file!",
                                  "The file you just opened is slightly on the large side of "
                                  "what we can handle comfortably (ideally you don't want to "
                                  "go for files that have more than ~7 500 lines).\n\n "
                                  "Feel free to try working on it, however, but "
                                  "be aware that response times may not be ideal.",
                                  GCMessageDialog::OKOnly,
                                  GCMessageDialog::OK,
                                  GCMessageDialog::Warning );
  }
  else if( fileSize > DOMLIMIT )
  {
    m_DOMTooLarge = true;

    /* There is nothing to be acted upon for this particular message so no need
      to check what value the form returns. */ 
    GCMessageSpace::userAccepted( "VeryLargeFileWarning",
                                  "Very Large file!",
                                  "The file you just opened is too large for us "
                                  "to handle comfortably (ideally you don't want to "
                                  "go for files that have more than ~7 500 lines).\n\n "
                                  "Feel free to try working on it, however, but "
                                  "you definitely won't be able to see your changes "
                                  "in the text edit and depending on how large your file really "
                                  "is, things may also become impossibly slow.",
                                  GCMessageDialog::OKOnly,
                                  GCMessageDialog::OK,
                                  GCMessageDialog::Warning );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::insertEmptyTableRow()
{
  QTableWidgetItem *label = new QTableWidgetItem( EMPTY );

  int lastRow = ui->tableWidget->rowCount();
  ui->tableWidget->setRowCount( lastRow + 1 );
  ui->tableWidget->setItem( lastRow, 0, label );

  /* Create the combo box, but deactivate it until we have an associated attribute name. */
  GCComboBox *attributeCombo = new GCComboBox;
  attributeCombo->setEditable( true );
  attributeCombo->setEnabled( false );

  /* Only connect after inserting items or bad things will happen! */
  connect( attributeCombo, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( attributeValueChanged( QString ) ) );

  ui->tableWidget->setCellWidget( lastRow, 1, attributeCombo );
  m_comboBoxes.insert( attributeCombo, lastRow );

  /* This will point the current combo box member to the combo that's been activated
    in the table widget (used in "attributeValueChanged" to obtain the row number the
    combo box appears in in the table widget, etc, etc). */
  connect( attributeCombo, SIGNAL( activated( int ) ), m_signalMapper, SLOT( map() ) );
  m_signalMapper->setMapping( attributeCombo, attributeCombo );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::resetTableWidget()
{
  /* Remove the currently visible/live combo boxes from the signal mapper's
    mappings and the combo box map before we whack them all. */
  for( int i = 0; i < m_comboBoxes.keys().size(); ++i )
  {
    m_signalMapper->removeMappings( m_comboBoxes.keys().at( i ) );
  }

  m_comboBoxes.clear();

  ui->tableWidget->clearContents();   // also deletes current items
  ui->tableWidget->setRowCount( 0 );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::clearElementInfo()
{
  foreach( GCDomElementInfo *info, m_elementInfo.values() )
  {
    delete info;
    info = NULL;
  }

  m_elementInfo.clear();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::startSaveTimer()
{
  /* Automatically save the file at five minute intervals. */
  if( !m_saveTimer )
  {
    m_saveTimer = new QTimer( this );
    connect( m_saveTimer, SIGNAL( timeout() ), this, SLOT( saveXMLFile() ) );
    m_saveTimer->start( 300000 );
  }
  else
  {
    /* If the timer was stopped due to a DOM reset, start it again. */
    m_saveTimer->start( 300000 );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::toggleAddElementWidgets()
{
  /* Make sure we don't inadvertently create "empty" elements. */
  if( ui->addElementComboBox->count() < 1 )
  {
    ui->addElementComboBox->setEnabled( false );
    ui->addChildElementButton->setEnabled( false );
  }
  else
  {
    ui->addElementComboBox->setEnabled( true );
    ui->addChildElementButton->setEnabled( true );
  }
}

/*--------------------------------------------------------------------------------------*/
