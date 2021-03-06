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

#include "gcmainwindow.h"
#include "ui_gcmainwindow.h"
#include "db/gcdatabaseinterface.h"
#include "db/gcdbsessionmanager.h"
#include "forms/gcadditemsform.h"
#include "forms/gcremoveitemsform.h"
#include "forms/gchelpdialog.h"
#include "forms/gcsearchform.h"
#include "forms/gcaddsnippetsform.h"
#include "forms/gcrestorefilesform.h"
#include "utils/gctreewidgetitem.h"
#include "utils/gccombobox.h"
#include "utils/gcmessagespace.h"
#include "utils/gcglobalspace.h"

#include <QDesktopServices>
#include <QSignalMapper>
#include <QDomDocument>
#include <QTextBlock>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QTextCursor>
#include <QComboBox>
#include <QTimer>
#include <QLabel>
#include <QUrl>
#include <QCloseEvent>
#include <QFont>
#include <QScrollBar>
#include <QMovie>
#include <QSettings>

/*--------------------------------------------------------------------------------------*/

const QString EMPTY( "---" );
const QString LEFTRIGHTBRACKETS( "\\[|\\]" );

const qint64 DOMWARNING( 262144 );  // 0.25MB or ~7 500 lines
const qint64 DOMLIMIT  ( 524288 );  // 0.5MB  or ~15 000 lines

const int ATTRIBUTECOLUMN = 0;
const int VALUESCOLUMN = 1;

/*--------------------------------- MEMBER FUNCTIONS ----------------------------------*/

GCMainWindow::GCMainWindow( QWidget* parent )
: QMainWindow               ( parent ),
  ui                        ( new Ui::GCMainWindow ),
  m_signalMapper            ( new QSignalMapper( this ) ),
  m_activeAttribute         ( NULL ),
  m_currentCombo            ( NULL ),
  m_saveTimer               ( NULL ),
  m_activeProfileLabel      ( NULL ),
  m_progressLabel           ( NULL ),
  m_spinner                 ( NULL ),
  m_currentXMLFileName      ( "" ),
  m_activeAttributeName     ( "" ),
  m_wasTreeItemActivated    ( false ),
  m_newAttributeAdded       ( false ),
  m_busyImporting           ( false ),
  m_fileContentsChanged     ( false ),
  m_comboBoxes              ()
{
  ui->setupUi( this );
  ui->showEmptyProfileHelpButton->setVisible( false );
  ui->tableWidget->setFont( QFont( GCGlobalSpace::FONT, GCGlobalSpace::FONTSIZE ) );
  ui->tableWidget->horizontalHeader()->setFont( QFont( GCGlobalSpace::FONT, GCGlobalSpace::FONTSIZE ) );

  /* XML File related. */
  connect( ui->actionNew, SIGNAL( triggered() ), this, SLOT( newXMLFile() ) );
  connect( ui->actionOpen, SIGNAL( triggered() ), this, SLOT( openXMLFile() ) );
  connect( ui->actionSave, SIGNAL( triggered() ), this, SLOT( saveXMLFile() ) );
  connect( ui->actionSaveAs, SIGNAL( triggered() ), this, SLOT( saveXMLFileAs() ) );
  connect( ui->actionCloseFile, SIGNAL( triggered() ), this, SLOT( closeXMLFile() ) );

  /* Build/Edit XML. */
  connect( ui->addChildElementButton, SIGNAL( clicked() ), this, SLOT( addElementToDocument() ) );
  connect( ui->addSnippetButton, SIGNAL( clicked() ), this, SLOT( addSnippetToDocument() ) );

  /* Various other actions. */
  connect( ui->actionExit, SIGNAL( triggered() ), this, SLOT( close() ) );
  connect( ui->actionFind, SIGNAL( triggered() ), this, SLOT( searchDocument() ) );
  connect( ui->actionForgetPreferences, SIGNAL( triggered() ), this, SLOT( forgetMessagePreferences() ) );
  connect( ui->actionHelpContents, SIGNAL( triggered() ), this, SLOT( showMainHelp() ) );
  connect( ui->actionVisitOfficialSite, SIGNAL( triggered() ), this, SLOT( goToSite() ) );
  connect( ui->expandAllCheckBox, SIGNAL( clicked( bool ) ), this, SLOT( collapseOrExpandTreeWidget( bool ) ) );
  connect( ui->commentLineEdit, SIGNAL( textEdited( QString ) ), this, SLOT( updateComment( QString ) ) );
  connect( ui->actionUseDarkTheme, SIGNAL( triggered( bool ) ), this, SLOT( useDarkTheme( bool ) ) );

  connect( ui->wrapTextCheckBox, SIGNAL( clicked( bool ) ), ui->dockWidgetTextEdit, SLOT( wrapText( bool ) ) );
  connect( ui->dockWidgetTextEdit, SIGNAL( selectedIndex( int ) ), ui->treeWidget, SLOT( setCurrentItemFromIndex( int ) ) );
  connect( ui->dockWidgetTextEdit, SIGNAL( commentOut( const QList< int >&, const QString& ) ), this, SLOT( commentOut( const QList< int >&, const QString& ) ) );
  connect( ui->dockWidgetTextEdit, SIGNAL( manualEditAccepted() ), this, SLOT( rebuild() ) );

  /* Help related. */
  connect( ui->actionShowHelpButtons, SIGNAL( triggered( bool ) ), this, SLOT( setShowHelpButtons( bool ) ) );
  connect( ui->showEmptyProfileHelpButton, SIGNAL( clicked() ), this, SLOT( showEmptyProfileHelp() ) );
  connect( ui->showAddElementHelpButton, SIGNAL( clicked() ), this, SLOT( showElementHelp() ) );

  /* Everything tree widget related. */
  connect( ui->treeWidget, SIGNAL( gcCurrentItemSelected( GCTreeWidgetItem*, int ) ), this, SLOT( elementSelected( GCTreeWidgetItem*, int ) ) );
  connect( ui->treeWidget, SIGNAL( gcCurrentItemChanged( GCTreeWidgetItem*, int ) ), this, SLOT( elementChanged( GCTreeWidgetItem*, int ) ) );
  connect( ui->treeWidget, SIGNAL( collapsed( QModelIndex ) ), this, SLOT( uncheckExpandAll() ) );
  connect( ui->actionShowTreeElementsVerbose, SIGNAL( triggered( bool ) ), this, SLOT( setShowTreeItemsVerbose( bool ) ) );

  /* Everything table widget related. */
  connect( ui->tableWidget, SIGNAL( itemClicked( QTableWidgetItem* ) ), this, SLOT( attributeSelected( QTableWidgetItem* ) ) );
  connect( ui->tableWidget, SIGNAL( itemChanged( QTableWidgetItem* ) ), this, SLOT( attributeChanged( QTableWidgetItem* ) ) );

  /* Database related. */
  connect( ui->actionAddItems, SIGNAL( triggered() ), this, SLOT( addItemsToDB() ) );
  connect( ui->actionRemoveItems, SIGNAL( triggered() ), this, SLOT( removeItemsFromDB() ) );
  connect( ui->actionImportXMLToDatabase, SIGNAL( triggered() ), this, SLOT( importXMLFromFile() ) );
  connect( ui->actionSwitchSessionDatabase, SIGNAL( triggered() ), this, SLOT( switchActiveDatabase() ) );
  connect( ui->actionAddNewDatabase, SIGNAL( triggered() ), this, SLOT( addNewDatabase() ) );
  connect( ui->actionAddExistingDatabase, SIGNAL( triggered() ), this, SLOT( addExistingDatabase() ) );
  connect( ui->actionRemoveDatabase, SIGNAL( triggered() ), this, SLOT( removeDatabase() ) );

  connect( m_signalMapper, SIGNAL( mapped( QWidget* ) ), this, SLOT( setCurrentComboBox( QWidget* ) ) );

  readSettings();

  /* Wait for the event loop to be initialised before calling this function. */
  QTimer::singleShot( 0, this, SLOT( initialise() ) );
}

/*--------------------------------------------------------------------------------------*/

GCMainWindow::~GCMainWindow()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::closeEvent( QCloseEvent* event )
{
  if( m_fileContentsChanged )
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

  deleteTempFile();
  saveSettings();
  QMainWindow::closeEvent( event );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::initialise()
{
  /* Initialise the database interface and retrieve the list of database names (this will
    include the path references to the ".db" files). */
  if( !GCDataBaseInterface::instance()->isInitialised() )
  {
    GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
    this->close();
  }

  /* If the interface was successfully initialised, prompt the user to choose a database
    connection for this session. */
  GCDBSessionManager* manager = createDBSessionManager();
  manager->selectActiveDatabase();
  QDialog::DialogCode result = static_cast< QDialog::DialogCode >( manager->result() );

  if( result == QDialog::Rejected )
  {
    this->close();    // also deletes manager ("this" is parent)
  }

  queryRestoreFiles();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::elementChanged( GCTreeWidgetItem* item, int column )
{
  if( !ui->treeWidget->empty() )
  {
    elementSelected( item, column );
    setTextEditContent( item );
  }
  else
  {
    resetDOM();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::elementSelected( GCTreeWidgetItem* item, int column )
{
  Q_UNUSED( column );

  if( item )
  {
    /* This flag is set to prevent the functionality in "attributeChanged" (which is triggered
    by the population of the table widget) from being executed until this function exits. */
    m_wasTreeItemActivated = true;

    resetTableWidget();

    QDomElement element = item->element();      // shallow copy
    QString elementName = item->name();
    QStringList attributeNames = GCDataBaseInterface::instance()->attributes( elementName );

    /* Add all the associated attribute names to the first column of the table widget,
    create and populate combo boxes with the attributes' known values and insert the
    combo boxes into the second column of the table widget. Finally, insert an "empty"
    row so that the user may add additional attributes and values to the current element. */
    for( int i = 0; i < attributeNames.count(); ++i )
    {
      GCComboBox* attributeCombo = new GCComboBox;
      QTableWidgetItem* label = new QTableWidgetItem( attributeNames.at( i ) );
      label->setFlags( label->flags() | Qt::ItemIsUserCheckable );

      if( item->attributeIncluded( attributeNames.at( i ) ) )
      {
        label->setCheckState( Qt::Checked );
        attributeCombo->setEnabled( true );
      }
      else
      {
        label->setCheckState( Qt::Unchecked );
        attributeCombo->setEnabled( false );
      }

      ui->tableWidget->setRowCount( i + 1 );
      ui->tableWidget->setItem( i, 0, label );

      attributeCombo->addItems( GCDataBaseInterface::instance()->attributeValues( elementName, attributeNames.at( i ) ) );
      attributeCombo->setEditable( true );

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
        attributeCombo->setCurrentIndex( -1 );
      }

      /* Attempting the connection before we've set the current index above causes the
      "attributeValueChanged" slot to be called prematurely, resulting in a segmentation
      fault due to value conflicts/missing values (in short, we can't do the connect
      before we set the current index above). */
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
    ui->addElementComboBox->addItems( GCDataBaseInterface::instance()->children( elementName ) );

    /* The following will be used to allow the user to add an element of the current type to
    its parent (this should improve the user experience as they do not have to explicitly
    navigate back up to a parent when adding multiple items of the same type).  We also don't
    want the user to add a document root element to itself by accident. */
    if( !ui->treeWidget->matchesRootName( elementName ) )
    {
      ui->addElementComboBox->addItem( QString( "[%1]" ).arg( elementName ) );
    }

    toggleAddElementWidgets();
    highlightTextElement( item );

    /* The user must not be allowed to add an entire document as a "snippet". */
    if( ui->treeWidget->matchesRootName( ui->addElementComboBox->currentText() ) )
    {
      ui->addSnippetButton->setEnabled( false );
      ui->addChildElementButton->setText( "Add Root" );
    }
    else
    {
      ui->addSnippetButton->setEnabled( true );
      ui->addChildElementButton->setText( "Add Child" );
    }

    ui->commentLineEdit->setText( ui->treeWidget->activeCommentValue() );

    /* Unset flag. */
    m_wasTreeItemActivated = false;
  }
  else if( ui->treeWidget->empty() )
  {
    resetDOM();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::attributeChanged( QTableWidgetItem* tableItem )
{
  /* Don't execute the logic if a tree widget item's activation is triggering
    a re-population of the table widget (which results in this slot being called). */
  if( !m_wasTreeItemActivated && !m_newAttributeAdded )
  {
    /* Also don't allow for empty attribute names. */
    if( tableItem->text().isEmpty() )
    {
      tableItem->setText( m_activeAttributeName );
      return;
    }

    /* When the check state of a table widget item changes, the "itemChanged" signal is emitted
      before the "itemClicked" one and this messes up the logic that follows completely.  Since
      I depend on knowing which attribute is currently active, I needed to insert the following
      odd little check (the other alternative is probably to subclass QTableWidgetItem but I'm
      worried about the added complications). */
    if( m_activeAttribute != tableItem )
    {
      m_activeAttributeName = tableItem->text();
    }

    /* All attribute name changes will be assumed to be additions, removing an attribute
      with a specific name has to be done explicitly. */
    GCTreeWidgetItem* treeItem = ui->treeWidget->gcCurrentItem();

    /* See if an existing attribute's name changed or if a new attribute was added. */
    if( tableItem->text() != m_activeAttributeName )
    {
      /* Add the new attribute's name to the current element's list of associated attributes. */
      if( GCDataBaseInterface::instance()->updateElementAttributes( treeItem->name(), QStringList( tableItem->text() ) ) )
      {
        /* Is this a name change? */
        if( m_activeAttributeName != EMPTY )
        {
          treeItem->excludeAttribute( m_activeAttributeName );

          /* Retrieve the list of values associated with the previous name and insert it against the new name. */
          QStringList attributeValues = GCDataBaseInterface::instance()->attributeValues( treeItem->name(),
                                                                                          m_activeAttributeName );

          if( !GCDataBaseInterface::instance()->updateAttributeValues( treeItem->name(), tableItem->text(), attributeValues ) )
          {
            GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
          }
        }
        else
        {
          /* If this is an entirely new attribute, insert an "empty" row so that the user may add
            even more attributes if he/she wishes to do so. */
          m_newAttributeAdded = true;
          tableItem->setFlags( tableItem->flags() | Qt::ItemIsUserCheckable );
          tableItem->setCheckState( Qt::Checked );
          insertEmptyTableRow();
          m_newAttributeAdded = false;
        }
      }
      else
      {
        GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
      }
    }

    m_activeAttributeName = tableItem->text();

    /* Is this attribute included or excluded? */
    GCComboBox* attributeValueCombo = dynamic_cast< GCComboBox* >( ui->tableWidget->cellWidget( tableItem->row(), VALUESCOLUMN ) );

    if( tableItem->checkState() == Qt::Checked )
    {
      attributeValueCombo->setEnabled( true );
      treeItem->includeAttribute( m_activeAttributeName, attributeValueCombo->currentText() );
    }
    else
    {
      attributeValueCombo->setEnabled( false );
      treeItem->excludeAttribute( m_activeAttributeName );
    }

    setTextEditContent( treeItem );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::attributeSelected( QTableWidgetItem* tableItem )
{
  m_activeAttribute = tableItem;
  m_activeAttributeName = tableItem->text();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::attributeValueChanged( const QString& value )
{
  /* Don't execute the logic if a tree widget item's activation is triggering
    a re-population of the table widget (which results in this slot being called). */
  if( !m_wasTreeItemActivated && !value.isEmpty() )
  {
    GCTreeWidgetItem* treeItem = ui->treeWidget->gcCurrentItem();
    QString currentAttributeName = ui->tableWidget->item( m_comboBoxes.value( m_currentCombo ), ATTRIBUTECOLUMN )->text();

    /* If we don't know about this value, we need to add it to the DB. */
    QStringList attributeValues = GCDataBaseInterface::instance()->attributeValues( treeItem->name(), currentAttributeName );

    if( !attributeValues.contains( value ) )
    {
      if( !GCDataBaseInterface::instance()->updateAttributeValues( treeItem->name(),
                                                                   currentAttributeName,
                                                                   QStringList( value ) ) )
      {
        GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
      }
    }

    treeItem->includeAttribute( currentAttributeName, value );
    setTextEditContent( treeItem );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setCurrentComboBox( QWidget* combo )
{
  m_currentCombo = combo;
}

/*--------------------------------------------------------------------------------------*/

bool GCMainWindow::openXMLFile()
{
  /* Can't open a file if there is no DB profile to describe it. */
  querySetActiveSession( QString( "No active profile set, please set one for this session." ) );

  if( !queryResetDOM( "Save document before continuing?" ) )
  {
    return false;
  }

  /* Start off where the user finished last. */
  QString fileName = QFileDialog::getOpenFileName( this, "Open File", GCGlobalSpace::lastUserSelectedDirectory(), "XML Files (*.*)" );

  /* If the user cancelled, we don't want to continue. */
  if( fileName.isEmpty() )
  {
    return false;
  }

  /* Note to future self: although the user would have explicitly saved (or not saved) the file
    by the time this functionality is encountered, we only reset the document once we have a new,
    active file to work with since users are fickle and may still change their minds.  In other
    words, do NOT move this code to before "getOpenFileName" as previously considered! */
  resetDOM();
  m_currentXMLFileName = "";

  QFile file( fileName );

  if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
  {
    QString errorMsg = QString( "Failed to open file \"%1\": [%2]" )
      .arg( fileName )
      .arg( file.errorString() );
    GCMessageSpace::showErrorMessageBox( this, errorMsg );
    return false;
  }

  QTextStream inStream( &file );
  QString fileContent( inStream.readAll() );
  qint64 fileSize = file.size();
  file.close();

  /* This application isn't optimised for dealing with very large XML files (the entire point is that
    this suite should provide the functionality necessary for the manual manipulation of, e.g. XML config
    files normally set up by hand via copy and paste exercises), if this file is too large to be handled
    comfortably, we need to let the user know and also make sure that we don't try to set the DOM content
    as text in the QTextEdit (QTextEdit is optimised for paragraphs). */
  if( fileSize > DOMWARNING &&
      fileSize < DOMLIMIT )
  {
    QMessageBox::warning( this,
                          "Large file!",
                          "The file you just opened is pretty large. Response times may be slow." );
  }
  else if( fileSize > DOMLIMIT )
  {
    QMessageBox::critical( this,
                           "Very large file!",
                           "This file is too large to edit manually!" );

    return false;
  }

  QString xmlErr( "" );
  int line  ( -1 );
  int col   ( -1 );

  if( !ui->treeWidget->setContent( fileContent, &xmlErr, &line, &col ) )
  {
    QString errorMsg = QString( "XML is broken - Error [%1], line [%2], column [%3]" )
      .arg( xmlErr )
      .arg( line )
      .arg( col );
    GCMessageSpace::showErrorMessageBox( this, errorMsg );
    resetDOM();
    return false;
  }

  m_currentXMLFileName = fileName;
  m_fileContentsChanged = false;    // at first load, nothing has changed

  if( !m_busyImporting )
  {
    /* If the user is opening an XML file of a kind that isn't supported by the current active DB,
        we need to warn him/her of this fact and provide them with a couple of options. */
    if( !ui->treeWidget->documentCompatible() )
    {
      bool accepted = GCMessageSpace::userAccepted( "QueryImportXML",
                                                    "Import document?",
                                                    "Encountered unknown relationships - import differences to active profile?",
                                                    GCMessageSpace::YesNo,
                                                    GCMessageSpace::No,
                                                    GCMessageSpace::Question );

      if( accepted )
      {
        QTimer timer;
        timer.singleShot( 1000, this, SLOT( createSpinner() ) );
        qApp->processEvents( QEventLoop::ExcludeUserInputEvents );

        if( !ui->treeWidget->batchProcessSuccess() )
        {
          GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
        }
        else
        {
          qApp->processEvents( QEventLoop::ExcludeUserInputEvents );
          processDOMDoc();
        }

        deleteSpinner();
      }
      else
      {
        /* If the user decided to rather abort the import, reset everything. */
        resetDOM();
        m_currentXMLFileName = "";
      }
    }
    else
    {
      /* If the user selected a database that knows of this particular XML profile,
          simply process the document. */
      processDOMDoc();
    }
  }

  /* Save whatever directory the user ended up in. */
  QFileInfo fileInfo( fileName );
  QString finalDirectory = fileInfo.dir().path();
  GCGlobalSpace::setLastUserSelectedDirectory( finalDirectory );
  return true;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::newXMLFile()
{
  if( queryResetDOM( "Save document before continuing?" ) )
  {
    resetDOM();
    m_currentXMLFileName = "";
    ui->actionCloseFile->setEnabled( true );
    ui->actionSaveAs->setEnabled( true );
    ui->actionSave->setEnabled( true );
  }
}

/*--------------------------------------------------------------------------------------*/

bool GCMainWindow::saveXMLFile()
{
  if( m_currentXMLFileName.isEmpty() )
  {
    return saveXMLFileAs();
  }
  else
  {
    QFile file( m_currentXMLFileName );

    if( !file.open( QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text ) )
    {
      QString errMsg = QString( "Failed to save file \"%1\": [%2]." )
        .arg( m_currentXMLFileName )
        .arg( file.errorString() );
      GCMessageSpace::showErrorMessageBox( this, errMsg );
      return false;
    }
    else
    {
      QTextStream outStream( &file );
      outStream << ui->treeWidget->toString();
      file.close();

      m_fileContentsChanged = false;
      deleteTempFile();
      startSaveTimer();
    }
  }

  return true;
}

/*--------------------------------------------------------------------------------------*/

bool GCMainWindow::saveXMLFileAs()
{
  QString file = QFileDialog::getSaveFileName( this, "Save As", GCGlobalSpace::lastUserSelectedDirectory(), "XML Files (*.*)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    m_currentXMLFileName = file;

    /* Save the last visited directory. */
    QFileInfo fileInfo( m_currentXMLFileName );
    QString finalDirectory = fileInfo.dir().path();
    GCGlobalSpace::setLastUserSelectedDirectory( finalDirectory );

    return saveXMLFile();
  }
  else
  {
    /* Return false when the file save operation is cancelled so that "queryResetDom" does
      not inadvertently caused a file to be reset by accident when the user changes his/her mind. */
    return false;
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::closeXMLFile()
{
  if( queryResetDOM( "Save document before continuing?" ) )
  {
    resetDOM();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::importXMLFromFile()
{
  /* This flag is used in "openXMLFile" to distinguish between an explicit import
    and a simple file opening operation. */
  m_busyImporting = true;
  ui->treeWidget->setVisible( false );

  if( openXMLFile() )
  {
    if( importXMLToDatabase() )
    {
      if( m_progressLabel )
      {
        m_progressLabel->hide();
      }

      QMessageBox::StandardButtons accept = QMessageBox::question( this,
                                                                   "Edit file",
                                                                   "Also open file for editing?",
                                                                   QMessageBox::Yes | QMessageBox::No,
                                                                   QMessageBox::Yes );

      if( accept == QMessageBox::Yes )
      {
        QTimer timer;

        if( m_progressLabel )
        {
          timer.singleShot( 1000, m_progressLabel, SLOT( show() ) );
        }

        qApp->processEvents( QEventLoop::ExcludeUserInputEvents );
        processDOMDoc();
      }
      else
      {
        /* DOM was set in the process of opening the XML file and loading its content.  If the user
            doesn't want to work with the file that was imported, we need to reset it here. */
        resetDOM();
        m_currentXMLFileName = "";
      }
    }
  }

  ui->treeWidget->setVisible( true );
  m_busyImporting = false;
}

/*--------------------------------------------------------------------------------------*/

bool GCMainWindow::importXMLToDatabase()
{
  createSpinner();
  qApp->processEvents( QEventLoop::ExcludeUserInputEvents );

  if( !ui->treeWidget->batchProcessSuccess() )
  {
    GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->lastError() );
    deleteSpinner();
    return false;
  }

  deleteSpinner();
  return true;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addNewDatabase()
{
  GCDBSessionManager* manager = createDBSessionManager();

  /* If we have an active DOM document, we need to pass the name of the root
    element through to the DB session manager which uses it to determine whether
    or not a user is on the verge of messing something up... */
  if( ui->treeWidget->empty() )
  {
    manager->addNewDatabase();
  }
  else
  {
    manager->addNewDatabase( ui->treeWidget->rootName() );
  }

  delete manager;

  /* Ensure that the user sets a database as active for this session. */
  querySetActiveSession( QString( "No active profile set, please set one for this session." ) );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addExistingDatabase()
{
  GCDBSessionManager* manager = createDBSessionManager();

  /* If we have an active DOM document, we need to pass the name of the root
    element through to the DB session manager which uses it to determine whether
    or not a user is on the verge of messing something up... */
  if( ui->treeWidget->empty() )
  {
    manager->addExistingDatabase();
  }
  else
  {
    manager->addExistingDatabase( ui->treeWidget->rootName() );
  }

  delete manager;

  /* Ensure that the user sets a database as active for this session. */
  querySetActiveSession( QString( "No active profile set, please set one for this session." ) );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::removeDatabase()
{
  GCDBSessionManager* manager = createDBSessionManager();

  /* If we have an active DOM document, we need to pass the name of the root
    element through to the DB session manager which uses it to determine whether
    or not a user is on the verge of messing something up... */
  if( ui->treeWidget->empty() )
  {
    manager->removeDatabase();
  }
  else
  {
    manager->removeDatabase( ui->treeWidget->rootName() );
  }

  delete manager;

  /* If the user removed the active DB for this session, we need to know
    what he/she intends to replace it with. */
  querySetActiveSession( QString( "The active profile has been removed, please select another." ) );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::switchActiveDatabase()
{
  GCDBSessionManager* manager = createDBSessionManager();

  /* If we have an active DOM document, we need to pass the name of the root
    element through to the DB session manager which uses it to determine whether
    or not a user is on the verge of messing something up... */
  if( ui->treeWidget->empty() )
  {
    manager->selectActiveDatabase();
  }
  else
  {
    manager->selectActiveDatabase( ui->treeWidget->rootName() );
  }

  delete manager;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::activeDatabaseChanged( QString dbName )
{
  if( ui->treeWidget->empty() )
  {
    resetDOM();
  }

  /* If the user set an empty database, prompt to populate it.  This message must
    always be shown (i.e. we don't have to show the custom dialog box that provides
    the \"Don't show this again\" option). */
  if( GCDataBaseInterface::instance()->isProfileEmpty() )
  {
    QMessageBox::StandardButton accepted = QMessageBox::warning( this,
                                                                 "Empty Profile",
                                                                 "Empty profile selected. Import XML from file?",
                                                                 QMessageBox::Yes | QMessageBox::No,
                                                                 QMessageBox::Yes );

    if( accepted == QMessageBox::Yes )
    {
      importXMLFromFile();
    }
    else
    {
      addItemsToDB();
    }
  }

  if( !m_activeProfileLabel )
  {
    m_activeProfileLabel = new QLabel( QString( "Active Profile: %1" ).arg( dbName ) );
    statusBar()->addWidget( m_activeProfileLabel );
  }
  else
  {
    m_activeProfileLabel->setText( QString( "Active Profile: %1" ).arg( dbName ) );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addElementToDocument()
{
  QString elementName = ui->addElementComboBox->currentText();
  bool treeWasEmpty = ui->treeWidget->empty();
  bool addToParent = false;

  /* If the user selected the <element> option, we add a new sibling element
    of the same name as the current element to the current element's parent. */
  if( elementName.contains( QRegExp( LEFTRIGHTBRACKETS ) ) )
  {
    elementName = elementName.remove( QRegExp( LEFTRIGHTBRACKETS ) );
    addToParent = true;
  }

  /* There is probably no chance of this ever happening, but defensive programming FTW! */
  if( !elementName.isEmpty() )
  {
    /* Update the tree widget. */
    ui->treeWidget->addItem( elementName, addToParent );

    GCTreeWidgetItem* treeItem = ui->treeWidget->gcCurrentItem();
    treeItem->setFlags( treeItem->flags() | Qt::ItemIsEditable );

    /* If the user starts creating a DOM document without having explicitly asked for
    a new file to be created, do it automatically (we can't call "newXMLFile here" since
    it resets the DOM document). */
    if( treeWasEmpty )
    {
      m_currentXMLFileName = "";
      ui->actionCloseFile->setEnabled( true );
      ui->actionSave->setEnabled( true );
      ui->actionSaveAs->setEnabled( true );
      ui->addSnippetButton->setEnabled( true );
      ui->addChildElementButton->setText( "Add Child" );
    }

    /* Add all the known attributes associated with this element name to the new element. */
    QStringList attributes = GCDataBaseInterface::instance()->attributes( elementName );

    for( int i = 0; i < attributes.size(); ++i )
    {
      treeItem->element().setAttribute( attributes.at( i ), QString( "" ) );
    }

    setTextEditContent( treeItem );
    elementSelected( treeItem, 0 );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addSnippetToDocument()
{
  QString elementName = ui->addElementComboBox->currentText();

  /* Check if we're inserting snippets as children, or as siblings. */
  if( elementName.contains( QRegExp( LEFTRIGHTBRACKETS ) ) )
  {
    /* Qt::WA_DeleteOnClose flag set. */
    GCAddSnippetsForm* dialog = new GCAddSnippetsForm( elementName.remove( QRegExp( LEFTRIGHTBRACKETS ) ),
                                                       ui->treeWidget->gcCurrentItem()->gcParent(),
                                                       this );
    connect( dialog, SIGNAL( snippetAdded( GCTreeWidgetItem*, QDomElement ) ), this, SLOT( insertSnippet( GCTreeWidgetItem*, QDomElement ) ) );
    dialog->exec();
  }
  else
  {
    /* Qt::WA_DeleteOnClose flag set. */
    GCAddSnippetsForm* dialog = new GCAddSnippetsForm( elementName,
                                                       ui->treeWidget->gcCurrentItem(),
                                                       this );
    connect( dialog, SIGNAL( snippetAdded( GCTreeWidgetItem*, QDomElement ) ), this, SLOT( insertSnippet( GCTreeWidgetItem*, QDomElement ) ) );
    dialog->exec();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::insertSnippet( GCTreeWidgetItem* treeItem, QDomElement element )
{
  ui->treeWidget->appendSnippet( treeItem, element );
  ui->treeWidget->expandAll();
  setTextEditContent();
  itemFound( treeItem->gcChild( treeItem->childCount() - 1 ) );
  m_fileContentsChanged = true;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::removeItemsFromDB()
{
  if( !GCDataBaseInterface::instance()->hasActiveSession() )
  {
    GCMessageSpace::showErrorMessageBox( this, "No active profile." );
    return;
  }

  if( GCDataBaseInterface::instance()->isProfileEmpty() )
  {
    QMessageBox::warning( this,
                          "Profile Empty",
                          "Active profile empty, nothing to remove." );
    return;
  }

  if( !ui->treeWidget->empty() )
  {
    GCMessageSpace::showErrorMessageBox( this, "For practical reasons, items cannot be removed when documents are open.\n"
                                         "Please \"Save\" and/or \"Close\" the current document before returning back here." );
    return;
  }

  /* Delete on close flag set (no clean-up needed). */
  GCRemoveItemsForm* dialog = new GCRemoveItemsForm( this );
  dialog->exec();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addItemsToDB()
{
  if( !GCDataBaseInterface::instance()->hasActiveSession() )
  {
    GCMessageSpace::showErrorMessageBox( this, "No active profile, please set one (Edit->Switch Profile)." );
    return;
  }

  bool profileWasEmpty = GCDataBaseInterface::instance()->isProfileEmpty();

  /* Delete on close flag set (no clean-up needed). */
  GCAddItemsForm* form = new GCAddItemsForm( this );
  form->exec();

  /* If the active profile has just been populated with elements for the first time,
    make sure that we set the newly added root elements to the dropdown. */
  if( profileWasEmpty )
  {
    ui->addElementComboBox->addItems( GCDataBaseInterface::instance()->knownRootElements() );
    toggleAddElementWidgets();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::searchDocument()
{
  /* Delete on close flag set (no clean-up needed). */
  GCSearchForm* form = new GCSearchForm( ui->treeWidget->allTreeWidgetItems(), ui->dockWidgetTextEdit, this );
  connect( form, SIGNAL( foundItem( GCTreeWidgetItem* ) ), this, SLOT( itemFound( GCTreeWidgetItem* ) ) );
  form->exec();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::itemFound( GCTreeWidgetItem* item )
{
  ui->treeWidget->expandAll();
  ui->treeWidget->setCurrentItem( item );
  elementSelected( item, 0 );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::commentOut( const QList< int >& indices, const QString& comment )
{
  m_fileContentsChanged = true;
  ui->treeWidget->replaceItemsWithComment( indices, comment );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::rebuild()
{
  /* No need to check if setContent is a success.  If this function gets called, the document
    content is already valid XML. The reason I reset the text edit's content is due to the Qt
    XML parser adding attributes in alphabetical order.  What this means is that the elements
    in the tree widget have all their attributes aligned alphabetically, and this may or may
    not add up with what is in the text edit, hence the reset (this way we ensure that the order
    of the attributes in the text edit matches exactly that of the tree widget's elements). */
  ui->treeWidget->setContent( ui->dockWidgetTextEdit->toPlainText() );
  ui->dockWidgetTextEdit->setContent( ui->treeWidget->toString() );

  ui->treeWidget->expandAll();
  m_fileContentsChanged = true;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::updateComment( const QString& comment )
{
  ui->treeWidget->setActiveCommentValue( comment );
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

void GCMainWindow::uncheckExpandAll()
{
  ui->expandAllCheckBox->setChecked( false );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::forgetMessagePreferences()
{
  GCMessageSpace::forgetAllPreferences();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::createSpinner()
{
  /* Clean-up must be handled in the calling function, but just in case. */
  if( m_spinner || m_progressLabel )
  {
    deleteSpinner();
  }

  m_progressLabel = new QLabel( this, Qt::Popup );
  m_progressLabel->move( window()->frameGeometry().topLeft() + window()->rect().center() - m_progressLabel->rect().center() );

  m_spinner = new QMovie( ":/resources/spinner.gif" );
  m_spinner->start();

  m_progressLabel->setMovie( m_spinner );
  m_progressLabel->show();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::deleteSpinner()
{
  if( m_spinner )
  {
    delete m_spinner;
    m_spinner = NULL;
  }

  if( m_progressLabel )
  {
    delete m_progressLabel;
    m_progressLabel = NULL;
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::resetDOM()
{
  ui->treeWidget->clearAndReset();
  ui->dockWidgetTextEdit->clearAndReset();

  resetTableWidget();

  ui->addElementComboBox->clear();
  ui->addElementComboBox->addItems( GCDataBaseInterface::instance()->knownRootElements() );
  toggleAddElementWidgets();

  ui->addSnippetButton->setEnabled( false );
  ui->addChildElementButton->setText( "Add Root" );

  m_currentCombo = NULL;
  m_activeAttributeName = "";
  m_fileContentsChanged = false;

  /* The timer will be reactivated as soon as work starts again on a legitimate
    document and the user saves it for the first time. */
  if( m_saveTimer )
  {
    m_saveTimer->stop();
  }
}

/*--------------------------------------------------------------------------------------*/

bool GCMainWindow::queryResetDOM( const QString& resetReason )
{
  /* There are a number of places and opportunities for "resetDOM" to be called,
    if there is an active document, check if it's content has changed since the
    last time it had changed and make sure we don't accidentally delete anything. */
  if( m_fileContentsChanged )
  {
    QMessageBox::StandardButtons accept = QMessageBox::question( this,
                                                                 "Save file?",
                                                                 resetReason,
                                                                 QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                                                 QMessageBox::Yes );

    if( accept == QMessageBox::Yes )
    {
      return saveXMLFile();
    }
    else if( accept == QMessageBox::No )
    {
      m_fileContentsChanged = false;
    }
    else
    {
      return false;
    }
  }

  return true;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showEmptyProfileHelp()
{
  QMessageBox::information( this,
                            "Empty Profile",
                            "The active profile is empty.  You can either import XML from file "
                            "via \"Edit -> Import XML to Profile\" or you can populate the "
                            "profile from scratch via \"Edit -> Edit Profile -> Add Items\"." );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showElementHelp()
{
  QMessageBox::information( this,
                            "Adding Elements and Snippets",
                            "If the document is still empty, your first element will be the root element.\n\n"
                            "New elements are added as children of the element selected in the element tree.\n\n"
                            "\"Empty\" duplicate siblings are added by selecting the element name in the drop down "
                            "that matches that of the element selected in the element tree (these names will be "
                            "bracketed by \"[]\").\n\n"
                            "\"Snippets\" are fully formed XML segments consisting of the entire element hierarchy "
                            "with the element selected in the drop down combo box as base. Selecting this option will "
                            "provide you with the opportunity to provide default values for the snippet's attributes." );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showMainHelp()
{
  QFile file( ":/resources/help/Help.txt" );

  if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
  {
    GCMessageSpace::showErrorMessageBox( this, QString( "Failed to open \"Help\" file: [%1]" ).arg( file.errorString() ) );
  }
  else
  {
    QTextStream stream( &file );
    QString fileContent = stream.readAll();
    file.close();

    /* Qt::WA_DeleteOnClose flag set...no cleanup required. */
    GCHelpDialog* dialog = new GCHelpDialog( fileContent, this );
    dialog->show();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::goToSite()
{
  QDesktopServices::openUrl( QUrl( "http://goblincoding.com" ) );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::useDarkTheme( bool dark )
{
  if( dark )
  {
    QFile file( ":resources/StyleSheet.txt" );
    file.open( QIODevice::ReadOnly | QIODevice::Text );
    QTextStream stream( &file );

    qApp->setStyleSheet( stream.readAll() );
    file.close();
  }
  else
  {
    qApp->setStyleSheet( QString() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setShowHelpButtons( bool show )
{
  GCGlobalSpace::setShowHelpButtons( show );
  ui->showAddElementHelpButton->setVisible( show );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setShowTreeItemsVerbose( bool verbose )
{
  GCGlobalSpace::setShowTreeItemsVerbose( verbose );
  ui->treeWidget->setShowTreeItemsVerbose( verbose );
}

/*--------------------------------------------------------------------------------------*/

GCDBSessionManager* GCMainWindow::createDBSessionManager()
{
  /* Clean-up is the responsibility of the calling function. */
  GCDBSessionManager* manager = new GCDBSessionManager( this );
  connect( manager, SIGNAL( reset() ), this, SLOT( resetDOM() ) );
  connect( manager, SIGNAL( activeDatabaseChanged( QString ) ), this, SLOT( activeDatabaseChanged( QString ) ) );
  return manager;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::processDOMDoc()
{
  ui->treeWidget->rebuildTreeWidget(); // also deletes current items
  resetTableWidget();

  ui->addSnippetButton->setEnabled( true );
  ui->addChildElementButton->setText( "Add Child" );

  /* Enable file save options. */
  ui->actionCloseFile->setEnabled( true );
  ui->actionSave->setEnabled( true );
  ui->actionSaveAs->setEnabled( true );

  /* Display the DOM content in the text edit. */
  setTextEditContent();

  /* Generally-speaking, we want the file contents changed flag to be set whenever
    the text edit content is set (this is done, not surprisingly, in "setTextEditContent"
    above).  However, whenever a DOM document is processed for the first time, nothing
    is changed in it, so to avoid the annoying "Save File" queries when nothing has
    been done yet, we unset the flag here. */
  m_fileContentsChanged = false;

  collapseOrExpandTreeWidget( ui->expandAllCheckBox->isChecked() );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setStatusBarMessage( const QString& message )
{
  Q_UNUSED( message );
  // TODO.
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setTextEditContent( GCTreeWidgetItem* item )
{
  m_fileContentsChanged = true;
  ui->dockWidgetTextEdit->setContent( ui->treeWidget->toString() );
  highlightTextElement( item );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::highlightTextElement( GCTreeWidgetItem* item )
{
  if( item )
  {
    QString stringToMatch = item->toString();
    int pos = ui->treeWidget->itemPositionRelativeToIdenticalSiblings( stringToMatch, item->index() );
    ui->dockWidgetTextEdit->findTextRelativeToDuplicates( stringToMatch, pos );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::insertEmptyTableRow()
{
  QTableWidgetItem* label = new QTableWidgetItem( EMPTY );

  int lastRow = ui->tableWidget->rowCount();
  ui->tableWidget->setRowCount( lastRow + 1 );
  ui->tableWidget->setItem( lastRow, 0, label );

  /* Create the combo box, but deactivate it until we have an associated attribute name. */
  GCComboBox* attributeCombo = new GCComboBox;
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

void GCMainWindow::startSaveTimer()
{
  /* Automatically save the file at five minute intervals. */
  if( !m_saveTimer )
  {
    m_saveTimer = new QTimer( this );
    connect( m_saveTimer, SIGNAL( timeout() ), this, SLOT( saveTempFile() ) );
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
    ui->commentLineEdit->setEnabled( false );

    /* Check if the element combo box is empty due to an empty profile
      being active. */
    if( GCDataBaseInterface::instance()->isProfileEmpty() )
    {
      ui->showEmptyProfileHelpButton->setVisible( true );
    }
    else
    {
      ui->showEmptyProfileHelpButton->setVisible( false );
    }
  }
  else
  {
    ui->addElementComboBox->setEnabled( true );
    ui->addChildElementButton->setEnabled( true );
    ui->commentLineEdit->setEnabled( true );
    ui->showEmptyProfileHelpButton->setVisible( false );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::querySetActiveSession( QString reason )
{
  while( !GCDataBaseInterface::instance()->hasActiveSession() )
  {
    GCMessageSpace::showErrorMessageBox( this, reason );
    GCDBSessionManager* manager = createDBSessionManager();
    manager->selectActiveDatabase();
    delete manager;
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::readSettings()
{
  restoreGeometry( GCGlobalSpace::windowGeometry() );
  restoreState( GCGlobalSpace::windowState() );

  setShowHelpButtons( GCGlobalSpace::showHelpButtons() );
  setShowTreeItemsVerbose( GCGlobalSpace::showTreeItemsVerbose() );

  ui->actionRememberWindowGeometry->setChecked( GCGlobalSpace::useWindowSettings() );
  ui->actionUseDarkTheme->setChecked( GCGlobalSpace::useDarkTheme() );
  ui->actionShowHelpButtons->setChecked( GCGlobalSpace::showHelpButtons() );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::saveSettings()
{
  if( ui->actionRememberWindowGeometry->isChecked() )
  {
    GCGlobalSpace::setWindowGeometry( saveGeometry() );
    GCGlobalSpace::setWindowState( saveState() );
  }
  else
  {
    GCGlobalSpace::removeWindowInfo();
  }

  GCGlobalSpace::setUseDarkTheme( ui->actionUseDarkTheme->isChecked() );
  GCGlobalSpace::setUseWindowSettings( ui->actionRememberWindowGeometry->isChecked() );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::queryRestoreFiles()
{
  QDir dir = QDir::current();
  QStringList tempFiles = QDir::current()
    .entryList( QDir::Files )
    .filter( QString( "%1_temp" )
             .arg( GCDataBaseInterface::instance()->activeSessionName().remove( ".db" ) ) );

  if( !tempFiles.empty() )
  {
    QMessageBox::StandardButton accept = QMessageBox::information( this,
                                                                   "Found recovery files",
                                                                   "Found auto-recover files related to this profile. Would you like to view and save?",
                                                                   QMessageBox::Ok | QMessageBox::Cancel,
                                                                   QMessageBox::Ok );

    if( accept == QMessageBox::Ok )
    {
      /* "Delete on close" flag set. */
      GCRestoreFilesForm* restore = new GCRestoreFilesForm( tempFiles, this );
      restore->exec();
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::saveTempFile()
{
  QFile file( QDir::currentPath() +
              QString( "/%1_%2_temp" )
              .arg( m_currentXMLFileName.split( "/" ).last() )
              .arg( GCDataBaseInterface::instance()->activeSessionName().remove( ".db" ) ) );

  /* Since this is an attempt at auto-saving, we aim for a "best case" scenario and don't display
    error messages if encountered. */
  if( file.open( QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text ) )
  {
    QTextStream outStream( &file );
    outStream << ui->treeWidget->toString();
    file.close();
    startSaveTimer();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::deleteTempFile()
{
  QFile file( QDir::currentPath() +
              QString( "/%1_%2_temp" )
              .arg( m_currentXMLFileName.split( "/" ).last() )
              .arg( GCDataBaseInterface::instance()->activeSessionName().remove( ".db" ) ) );

  file.remove();
}

/*--------------------------------------------------------------------------------------*/