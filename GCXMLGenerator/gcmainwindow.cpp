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
#include "db/gcdbsessionmanager.h"
#include "xml/xmlsyntaxhighlighter.h"
#include "forms/gcadditemsform.h"
#include "forms/gcremoveitemsform.h"
#include "forms/gchelpdialog.h"
#include "forms/gcsearchform.h"
#include "forms/gcsnippetsform.h"
#include "forms/gcrestorefilesform.h"
#include "utils/gccombobox.h"
#include "utils/gcmessagespace.h"
#include "utils/gcdomelementinfo.h"
#include "utils/gcglobals.h"

#include <QDesktopServices>
#include <QSignalMapper>
#include <QDomDocument>
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
const qint64  DOMWARNING( 262144 );  // 0.25MB or ~7 500 lines
const qint64  DOMLIMIT  ( 524288 );  // 0.5MB  or ~15 000 lines

const int ATTRIBUTECOLUMN = 0;
const int VALUESCOLUMN    = 1;

/*--------------------------------- MEMBER FUNCTIONS ----------------------------------*/

GCMainWindow::GCMainWindow( QWidget *parent ) :
  QMainWindow           ( parent ),
  ui                    ( new Ui::GCMainWindow ),
  m_signalMapper        ( new QSignalMapper( this ) ),
  m_domDoc              ( new QDomDocument ),
  m_activeAttribute     ( NULL ),
  m_currentCombo        ( NULL ),
  m_saveTimer           ( NULL ),
  m_activeProfileLabel  ( NULL ),
  m_progressLabel       ( NULL ),
  m_spinner             ( NULL ),
  m_currentXMLFileName  ( "" ),
  m_activeAttributeName ( "" ),
  m_wasTreeItemActivated( false ),
  m_newAttributeAdded   ( false ),
  m_busyImporting       ( false ),
  m_fileContentsChanged ( false ),
  m_elementInfo         (),
  m_treeItemNodes       (),
  m_comboBoxes          ()
{
  ui->setupUi( this );
  ui->emptyProfileHelpButton->setVisible( false );
  ui->dockWidgetTextEdit->setFont( QFont( FONT, FONTSIZE ) );

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
  connect( ui->wrapTextCheckBox, SIGNAL( clicked( bool ) ), this, SLOT( wrapText( bool ) ) );
  connect( ui->emptyProfileHelpButton, SIGNAL( clicked() ), this, SLOT( showEmptyProfileHelp() ) );
  connect( ui->showCommentHelpButton, SIGNAL( clicked() ), this, SLOT( showCommentHelp() ) );
  connect( ui->commentLineEdit, SIGNAL( returnPressed() ), this, SLOT( addComment() ) );
  connect( ui->actionUseDarkTheme, SIGNAL( triggered( bool ) ), this, SLOT( useDarkTheme( bool ) ) );

  /* Everything tree widget related. */
  connect( ui->treeWidget, SIGNAL( itemClicked ( QTreeWidgetItem*, int ) ), this, SLOT( elementSelected( QTreeWidgetItem*, int ) ) );
  connect( ui->treeWidget, SIGNAL( itemActivated( QTreeWidgetItem*, int ) ), this, SLOT( elementSelected( QTreeWidgetItem*, int ) ) );
  connect( ui->treeWidget, SIGNAL( itemChanged( QTreeWidgetItem*, int ) ), this, SLOT( elementChanged( QTreeWidgetItem*, int ) ) );

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

  /* Everything happens automagically and the text edit takes ownership. */
  XmlSyntaxHighlighter *highLighter = new XmlSyntaxHighlighter( ui->dockWidgetTextEdit->document() );
  Q_UNUSED( highLighter );

  readSettings();

  /* Wait for the event loop to be initialised before calling this function. */
  QTimer::singleShot( 0, this, SLOT( initialise() ) );
}

/*--------------------------------------------------------------------------------------*/

GCMainWindow::~GCMainWindow()
{
  deleteElementInfo();
  delete m_domDoc;
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::closeEvent( QCloseEvent *event )
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
  if( !GCDataBaseInterface::instance()->initialised() )
  {
    GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
    this->close();
  }

  /* If the interface was successfully initialised, prompt the user to choose a database
    connection for this session. */
  GCDBSessionManager *manager = createDBSessionManager();
  manager->selectActiveDatabase();
  QDialog::DialogCode result = static_cast< QDialog::DialogCode >( manager->result() );

  if( result == QDialog::Rejected )
  {
    this->close();    // also deletes manager ("this" is parent)
  }

  queryRestoreFiles();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::elementChanged( QTreeWidgetItem *item, int column )
{
  /* This check is probably redundant, but rather safe than sorry... */
  if( m_treeItemNodes.contains( item ) )
  {
    QString newName  = item->text( column );
    QString previousName = m_treeItemNodes.value( item ).tagName();

    if( newName.isEmpty() )
    {
      /* Reset if the user failed to specify a valid name. */
      item->setText( column, previousName );
    }
    else
    {
      if( newName != previousName )
      {
        /* Update the element names in our active DOM doc (since "m_treeItemNodes"
          contains shallow copied QDomElements, the change will automatically
          be applied to the map's nodes as well) and the tree widget. */
        QDomNodeList elementList = m_domDoc->elementsByTagName( previousName );

        for( int i = 0; i < elementList.count(); ++i )
        {
          QDomElement element( elementList.at( i ).toElement() ); // shallow copy
          element.setTagName( newName );
          const_cast< QTreeWidgetItem* >( m_treeItemNodes.key( element ) )->setText( column, newName );
        }

        /* The name change may introduce a new element too so we can safely call "addElement" below as
          it doesn't do anything if the element already exists in the database, yet it will obviously
          add the element if it doesn't.  In the latter case, the children  and attributes associated with
          the old name will be assigned to the new element in the process. */
        bool success( false );
        QStringList attributes = GCDataBaseInterface::instance()->attributes( previousName, success );

        if( !success )
        {
          GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
        }

        QStringList children = GCDataBaseInterface::instance()->children( previousName, success );

        if( !success )
        {
          GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
        }

        if( !GCDataBaseInterface::instance()->addElement( newName, children, attributes ) )
        {
          GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
        }

        /* If we are, in fact, dealing with a new element, we also want the "new" element's associated attributes
          to be updated with the known values of these attributes. */
        foreach( QString attribute, attributes )
        {
          QStringList attributeValues = GCDataBaseInterface::instance()->attributeValues( previousName, attribute, success );

          if( !GCDataBaseInterface::instance()->updateAttributeValues( newName, attribute, attributeValues ) )
          {
            GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
          }
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

  /* This flag is set to prevent the functionality in "attributeChanged" (which is triggered
    by the population of the table widget) from being executed until this function exits. */
  m_wasTreeItemActivated = true;

  resetTableWidget();

  bool success( false );
  QDomElement element = m_treeItemNodes.value( item );      // shallow copy
  QString elementName = element.tagName();
  QStringList attributeNames = GCDataBaseInterface::instance()->attributes( elementName, success );

  /* This is more for debugging than for end-user functionality. */
  if( !success )
  {
    GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
  }

  /* Add all the associated attribute names to the first column of the table widget,
    create and populate combo boxes with the attributes' known values and insert the
    combo boxes into the second column of the table widget. Finally, insert an "empty"
    row so that the user may add additional attributes and values to the current element. */
  for( int i = 0; i < attributeNames.count(); ++i )
  {
    GCComboBox *attributeCombo = new GCComboBox;
    QTableWidgetItem *label = new QTableWidgetItem( attributeNames.at( i ) );
    label->setFlags( label->flags() | Qt::ItemIsUserCheckable );

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

    ui->tableWidget->setRowCount( i + 1 );
    ui->tableWidget->setItem( i, 0, label );

    attributeCombo->addItems( GCDataBaseInterface::instance()->attributeValues( elementName, attributeNames.at( i ), success ) );
    attributeCombo->setEditable( true );

    /* This is more for debugging than for end-user functionality. */
    if( !success )
    {
      GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
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
  ui->addElementComboBox->addItems( GCDataBaseInterface::instance()->children( elementName, success ) );

  /* This is more for debugging than for end-user functionality. */
  if( !success )
  {
    GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
  }

  /* The following will be used to allow the user to add an element of the current type to
    its parent (this should improve the user experience as they do not have to explicitly
    navigate back up to a parent when adding multiple items of the same type).  We also don't
    want the user to add a document root element to itself by accident. */
  if( elementName != m_domDoc->documentElement().tagName() )
  {
    ui->addElementComboBox->addItem( QString( "<%1>" ).arg( elementName) );
  }

  toggleAddElementWidgets();
  highlightTextElement( element );

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
    /* When the check state of a table widget item changes, the "itemChanged" signal is emitted
      before the "itemClicked" one and this messes up the logic that follows completely.  Since
      I depend on knowing which attribute is currently active, I needed to insert the following
      odd little check (the other alternative is probably to subclass QTableWidgetItem but I'm
      worried about the added complications). */
    if( m_activeAttribute != item )
    {
      m_activeAttributeName = item->text();
    }

    /* All attribute name changes will be assumed to be additions, removing an attribute
      with a specific name has to be done explicitly. */
    QTreeWidgetItem *treeItem = ui->treeWidget->currentItem();
    QDomElement currentElement = m_treeItemNodes.value( treeItem ); // shallow copy

    /* See if an existing attribute's name changed or if a new attribute was added. */
    if( item->text() != m_activeAttributeName )
    {
      /* Add the new attribute's name to the current element's list of associated attributes. */
      if( GCDataBaseInterface::instance()->updateElementAttributes( currentElement.tagName(), QStringList( item->text() ) ) )
      {
        /* Is this a name change? */
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
              GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
            }
          }
          else
          {
            GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
          }
        }
        else
        {
          /* If this is an entirely new attribute, insert an "empty" row so that the user may add
            even more attributes if he/she wishes to do so. */
          m_newAttributeAdded = true;
          item->setFlags( item->flags() | Qt::ItemIsUserCheckable );
          item->setCheckState( Qt::Checked );
          insertEmptyTableRow();
          m_newAttributeAdded = false;
        }
      }
      else
      {
        GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
      }
    }

    /* Is this attribute included or excluded? */
    GCComboBox *attributeValueCombo = dynamic_cast< GCComboBox* >( ui->tableWidget->cellWidget( item->row(), VALUESCOLUMN ) );
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
  m_activeAttribute = item;
  m_activeAttributeName = item->text();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::attributeValueChanged( const QString &value )
{
  /* Don't execute the logic if a tree widget item's activation is triggering
    a re-population of the table widget (which results in this slot being called). */
  if( !m_wasTreeItemActivated )
  {
    QTreeWidgetItem *currentItem = ui->treeWidget->currentItem();
    QDomElement currentElement = m_treeItemNodes.value( currentItem );  // shallow copy

    QString currentAttributeName = ui->tableWidget->item( m_comboBoxes.value( m_currentCombo ), ATTRIBUTECOLUMN )->text();
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
          GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
        }
      }
    }
    else
    {
      GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
    }

    setTextEditContent( currentElement );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setCurrentComboBox( QWidget *combo )
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

  QString fileName = QFileDialog::getOpenFileName( this, "Open File", QDir::homePath(), "XML Files (*.*)" );

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
  int     line  ( -1 );
  int     col   ( -1 );

  if( !m_domDoc->setContent( fileContent, &xmlErr, &line, &col ) )
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

  /* If the user is opening an XML file of a kind that isn't supported by the current active DB,
    we need to warn him/her of this fact and provide them with a couple of options. */
  if( !GCDataBaseInterface::instance()->knownRootElements().contains( m_domDoc->documentElement().tagName() ) )
  {
    /* If we're not already busy importing an XML file, check if the user maybe wants to do so. */
    if( !m_busyImporting )
    {
      bool accepted = GCMessageSpace::userAccepted( "QueryImportXML",
                                                    "Import document?",
                                                    "Unknown XML - import to active profile?",
                                                    GCMessageSpace::YesNo,
                                                    GCMessageSpace::No,
                                                    GCMessageSpace::Question );

      if( accepted )
      {
        QTimer timer;
        timer.singleShot( 1000, this, SLOT( createSpinner() ) );
        qApp->processEvents( QEventLoop::ExcludeUserInputEvents );

        if( !GCDataBaseInterface::instance()->batchProcessDOMDocument( m_domDoc ) )
        {
          GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
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
  }
  else
  {
    /* If the user selected a database that knows of this particular XML profile,
      simply process the document. */
    if( !m_busyImporting )
    {
      processDOMDoc();
    }
  }

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
      GCMessageSpace::showErrorMessageBox( this, errMsg );
    }
    else
    {
      QTextStream outStream( &file );
      outStream << m_domDoc->toString( 2 );
      file.close();

      m_fileContentsChanged = false;
      deleteTempFile();
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

void GCMainWindow::importXMLFromFile()
{
  /* This flag is used in "openXMLFile" to distinguish between an explicit import
    and a simple file opening operation. */
  m_busyImporting = true;

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
                                                                   "Also view file for editing?",
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

  m_busyImporting = false;
}

/*--------------------------------------------------------------------------------------*/

bool GCMainWindow::importXMLToDatabase()
{
  createSpinner();
  qApp->processEvents( QEventLoop::ExcludeUserInputEvents );

  if( !GCDataBaseInterface::instance()->batchProcessDOMDocument( m_domDoc ) )
  {
    GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
    deleteSpinner();
    return false;
  }

  deleteSpinner();
  return true;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addNewDatabase()
{
  GCDBSessionManager *manager = createDBSessionManager();

  /* If we have an active DOM document, we need to pass the name of the root
    element through to the DB session manager which uses it to determine whether
    or not a user is on the verge of messing something up... */
  if( m_domDoc->documentElement().isNull() )
  {
    manager->addNewDatabase();
  }
  else
  {
    manager->addNewDatabase( m_domDoc->documentElement().tagName() );
  }

  delete manager;

  /* Ensure that the user sets a database as active for this session. */
  querySetActiveSession( QString( "No active profile set, please set one for this session." ) );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addExistingDatabase()
{
  GCDBSessionManager *manager = createDBSessionManager();

  /* If we have an active DOM document, we need to pass the name of the root
    element through to the DB session manager which uses it to determine whether
    or not a user is on the verge of messing something up... */
  if( m_domDoc->documentElement().isNull() )
  {
    manager->addExistingDatabase();
  }
  else
  {
    manager->addExistingDatabase( m_domDoc->documentElement().tagName() );
  }

  delete manager;

  /* Ensure that the user sets a database as active for this session. */
  querySetActiveSession( QString( "No active profile set, please set one for this session." ) );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::removeDatabase()
{
  GCDBSessionManager *manager = createDBSessionManager();

  /* If we have an active DOM document, we need to pass the name of the root
    element through to the DB session manager which uses it to determine whether
    or not a user is on the verge of messing something up... */
  if( m_domDoc->documentElement().isNull() )
  {
    manager->removeDatabase();
  }
  else
  {
    manager->removeDatabase( m_domDoc->documentElement().tagName() );
  }

  delete manager;

  /* If the user removed the active DB for this session, we need to know
    what he/she intends to replace it with. */
  querySetActiveSession( QString( "The active profile has been removed, please select another." ) );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::switchActiveDatabase()
{
  GCDBSessionManager *manager = createDBSessionManager();

  /* If we have an active DOM document, we need to pass the name of the root
    element through to the DB session manager which uses it to determine whether
    or not a user is on the verge of messing something up... */
  if( m_domDoc->documentElement().isNull() )
  {
    manager->selectActiveDatabase();
  }
  else
  {
    manager->selectActiveDatabase( m_domDoc->documentElement().tagName() );
  }

  delete manager;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::activeDatabaseChanged( QString dbName )
{
  if( m_domDoc->documentElement().isNull() )
  {
    resetDOM();
  }

  /* If the user set an empty database, prompt to populate it.  This message must
    always be shown (i.e. we don't have to show the custom dialog box that provides
    the \"Don't show this again\" option). */
  if( GCDataBaseInterface::instance()->profileEmpty() )
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

void GCMainWindow::deleteElementFromDocument()
{
  QTreeWidgetItem *currentItem = ui->treeWidget->currentItem();
  QDomElement currentElement = m_treeItemNodes.value( currentItem );  // shallow copy

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
  QString elementName = ui->addElementComboBox->currentText();

  /* If we already have mapped element nodes, add the new item to whichever
    parent item is currently highlighted in the tree widget. */
  QTreeWidgetItem *parentItem( NULL );

  if( !m_treeItemNodes.isEmpty() )
  {
    parentItem = ui->treeWidget->currentItem();
  }

  /* If the user selected the <element> option, we add a new sibling element
    of the same name as the current element to the current element's parent. */
  if( elementName.contains( QRegExp( "<|>" ) ) )
  {
    elementName = elementName.remove( QRegExp( "<|>" ) );
    parentItem = parentItem->parent();
  }

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

      QDomElement parent = m_treeItemNodes.value( parentItem ); // shallow copy
      parent.appendChild( newElement );
    }
    else
    {
      /* If the user starts creating a DOM document without having explicitly asked for
      a new file to be created, do it automatically (we can't call "newXMLFile here" since
      it resets the DOM document). */
      m_currentXMLFileName = "";
      ui->actionCloseFile->setEnabled( true );
      ui->actionSave->setEnabled( true );
      ui->actionSaveAs->setEnabled( true );

      /* Since we don't have any existing nodes if we get to this point, we need to initialise
        the tree widget with its first item. */
      ui->treeWidget->invisibleRootItem()->addChild( newItem );  // takes ownership
      m_domDoc->appendChild( newElement );
      ui->addSnippetButton->setEnabled( true );
    }

    /* Add all the known attributes associated with this element name to the new element. */
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
      GCMessageSpace::showErrorMessageBox( this, GCDataBaseInterface::instance()->getLastError() );
    }

    /* Check if the user provided a comment. */
    if( !ui->commentLineEdit->text().isEmpty() )
    {
      QDomComment comment = m_domDoc->createComment( ui->commentLineEdit->text() );
      newElement.parentNode().insertBefore( comment, newElement );
      ui->commentLineEdit->clear();
    }

    /* Keep everything in sync in the maps. */
    m_treeItemNodes.insert( newItem, newElement );
    m_elementInfo.insert( newItem, new GCDomElementInfo( newElement ) );

    setTextEditContent( newElement );
    ui->treeWidget->setCurrentItem( newItem, 0 );
    elementSelected( newItem, 0 );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addSnippetToDocument()
{
  QString elementName = ui->addElementComboBox->currentText();

  /* Check if we're inserting snippets as children, or as siblings. */
  if( elementName.contains( QRegExp( "<|>" ) ) )
  {
    /* Qt::WA_DeleteOnClose flag set. */
    GCSnippetsForm *dialog = new GCSnippetsForm( elementName.remove( QRegExp( "<|>" ) ),
                                                 m_treeItemNodes.value( ui->treeWidget->currentItem()->parent() ),
                                                 this );
    connect( dialog, SIGNAL( snippetAdded() ), this, SLOT( insertSnippet() ) );
    dialog->exec();
  }
  else
  {
    /* Qt::WA_DeleteOnClose flag set. */
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

void GCMainWindow::removeItemsFromDB()
{
  if( !GCDataBaseInterface::instance()->hasActiveSession() )
  {
    GCMessageSpace::showErrorMessageBox( this, "No active profile." );
    return;
  }

  if( GCDataBaseInterface::instance()->profileEmpty() )
  {
    QMessageBox::warning( this,
                          "Profile Empty",
                          "Active profile empty, nothing to remove." );
    return;
  }

  if( !m_domDoc->documentElement().isNull() )
  {
    GCMessageSpace::showErrorMessageBox( this, "\"Save\" and \"Close\" the current document before continuing.");
    return;
  }

  /* Delete on close flag set (no clean-up needed). */
  GCRemoveItemsForm *dialog = new GCRemoveItemsForm( this );
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

  bool profileWasEmpty = GCDataBaseInterface::instance()->profileEmpty();

  /* Delete on close flag set (no clean-up needed). */
  GCAddItemsForm *form = new GCAddItemsForm( this );
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
  GCSearchForm *form = new GCSearchForm( m_treeItemNodes.values(), m_domDoc->toString(), this );
  connect( form, SIGNAL( foundElement( QDomElement ) ), this, SLOT( elementFound( QDomElement ) ) );
  form->exec();
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

  /* Create a temporary document so that we do not mess with the contents
    of the tree item node map and current DOM if the new XML is broken. */
  QDomDocument doc;
  if( !doc.setContent( ui->dockWidgetTextEdit->toPlainText(), &xmlErr, &line, &col ) )
  {
    QString errorMsg = QString( "XML is broken - Error [%1], line [%2], column [%3]." )
                       .arg( xmlErr )
                       .arg( line )
                       .arg( col );
    GCMessageSpace::showErrorMessageBox( this, errorMsg );

    /* Unfortunately the line number returned by the DOM doc doesn't match up with what's
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

    /* The reason we import the saved XML to the database is that the user may have
      added elements and/or attributes that we don't know about.  If we don't do this,
      the current document and active profile will be out of sync and all kinds of chaos
      will ensue. */
    importXMLToDatabase();
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

void GCMainWindow::wrapText( bool wrap )
{
  if( wrap )
  {
    ui->dockWidgetTextEdit->setLineWrapMode( QPlainTextEdit::WidgetWidth );
  }
  else
  {
    ui->dockWidgetTextEdit->setLineWrapMode( QPlainTextEdit::NoWrap );
  }
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
  m_domDoc->clear();
  m_treeItemNodes.clear();
  ui->treeWidget->clear();
  ui->dockWidgetTextEdit->clear();
  resetTableWidget();
  deleteElementInfo();

  ui->addElementComboBox->clear();
  ui->addElementComboBox->addItems( GCDataBaseInterface::instance()->knownRootElements() );
  toggleAddElementWidgets();

  ui->addSnippetButton->setEnabled( false );

  m_currentCombo = NULL;
  m_activeAttributeName = "";

  /* The timer will be reactivated as soon as work starts again on a legitimate
    document and the user saves it for the first time. */
  if( m_saveTimer )
  {
    m_saveTimer->stop();
  }
}

/*--------------------------------------------------------------------------------------*/

bool GCMainWindow::queryResetDOM( const QString &resetReason )
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
      saveXMLFile();
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

void GCMainWindow::showDOMEditHelp()
{
  QMessageBox::information( this,
                            "Direct Edits",
                            "Changes to manually edited XML can only be reverted BEFORE you save." );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showCommentHelp()
{
  QMessageBox::information( this,
                            "Adding Comments",
                            "Comments are automatically added to newly created elements (if provided).\n\n"
                            "To add a comment to an existing element, select the relevant element in the tree, "
                            "provide the comment text and press \"Enter\"." );
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

void GCMainWindow::addComment()
{
  if( !ui->commentLineEdit->text().isEmpty() )
  {
    QDomElement currentElement = m_treeItemNodes.value( ui->treeWidget->currentItem() );
    QDomComment comment = m_domDoc->createComment( ui->commentLineEdit->text() );
    currentElement.parentNode().insertBefore( comment, currentElement );
    ui->commentLineEdit->clear();
    setTextEditContent( currentElement );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::useDarkTheme( bool use )
{
  if( use )
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

GCDBSessionManager *GCMainWindow::createDBSessionManager()
{
  /* Clean-up is the responsibility of the calling function. */
  GCDBSessionManager *manager = new GCDBSessionManager( this );
  connect( manager, SIGNAL( reset() ), this, SLOT( resetDOM() ) );
  connect( manager, SIGNAL( activeDatabaseChanged( QString ) ), this, SLOT( activeDatabaseChanged( QString ) ) );
  return manager;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::processDOMDoc()
{
  ui->treeWidget->clear(); // also deletes current items
  m_treeItemNodes.clear();
  deleteElementInfo();
  resetTableWidget();

  QDomElement root = m_domDoc->documentElement();

  /* Set the document root as the first item in the tree widget. */
  QTreeWidgetItem *item = new QTreeWidgetItem;
  item->setText( 0, root.tagName() );
  item->setFlags( item->flags() | Qt::ItemIsEditable );

  ui->treeWidget->invisibleRootItem()->addChild( item );  // takes ownership
  m_treeItemNodes.insert( item, root );
  m_elementInfo.insert  ( item, new GCDomElementInfo( root ) );

  /* Now we can recursively stick the rest of the elements into the tree widget. */
  populateTreeWidget( root, item );
  ui->addSnippetButton->setEnabled( true );

  /* Enable file save options. */
  ui->actionCloseFile->setEnabled( true );
  ui->actionSave->setEnabled( true );
  ui->actionSaveAs->setEnabled( true );

  /* Display the DOM content in the text edit. */
  setTextEditContent( QDomElement() );

  /* Generally-speaking, we want the file contents changed flag to be set whenever
    the text edit content is set (this is done, not surprisingly, in "setTextEditContent"
    above).  However, whenever a DOM document is processed for the first time, nothing
    is changed in it, so to avoid the annoying "Save File" queries when nothing has
    been done yet, we unset the flag here. */
  m_fileContentsChanged = false;

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
    m_elementInfo.insert  ( item, new GCDomElementInfo( element ) );

    populateTreeWidget( element, item );
    element = element.nextSiblingElement();
  }

  qApp->processEvents( QEventLoop::ExcludeUserInputEvents );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setStatusBarMessage( const QString &message )
{
  // TODO.
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setTextEditContent( const QDomElement &element )
{
  m_fileContentsChanged = true;

  /* Squeezing every once of performance out of the text edit...this significantly speeds
    up the loading of large files. */
  ui->dockWidgetTextEdit->setUpdatesEnabled( false );
  ui->dockWidgetTextEdit->setPlainText( m_domDoc->toString( 2 ) );
  ui->dockWidgetTextEdit->setUpdatesEnabled( true );

  highlightTextElement( element );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::highlightTextElement( const QDomElement &element )
{
  if( !element.isNull() )
  {
    QString stringToMatch = m_elementInfo.value( m_treeItemNodes.key( element ) )->toString();
    QList< QTreeWidgetItem* > identicalItems = ui->treeWidget->findItems( element.tagName(), Qt::MatchExactly | Qt::MatchRecursive );
    QList< int > indices;

    /* If there are multiple nodes with the same element name (more likely than not), check which
      of these nodes are exact duplicates with regards to attributes, values, etc. */
    foreach( QTreeWidgetItem* item, identicalItems )
    {
      QString node = m_elementInfo.value( item )->toString();

      if( node == stringToMatch )
      {
        indices.append( m_elementInfo.value( item )->index() );
      }
    }

    /* Now that we have a list of all the indices matching identical nodes (indices are a rough
      indication of an element's position in the DOM and closely matches the "line numbers" of the
      items in the tree widget), we can determine the position of the selected DOM element relative
      to its doppelgangers and highlight its text representation in the text edit area. */
    qSort( indices.begin(), indices.end() );
    ui->dockWidgetTextEdit->moveCursor( QTextCursor::Start );

    int elementIndex = m_elementInfo.value( m_treeItemNodes.key( element ) )->index();

    for( int i = 0; i <= indices.indexOf( elementIndex ); ++i )
    {
      ui->dockWidgetTextEdit->find( stringToMatch );
    }

    ui->dockWidgetTextEdit->ensureCursorVisible();
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

void GCMainWindow::deleteElementInfo()
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
    ui->showCommentHelpButton->setEnabled( false );

    /* Check if the element combo box is empty due to an empty profile
      being active. */
    if( GCDataBaseInterface::instance()->profileEmpty() )
    {
      ui->emptyProfileHelpButton->setVisible( true );
    }
    else
    {
      ui->emptyProfileHelpButton->setVisible( false );
    }
  }
  else
  {
    ui->addElementComboBox->setEnabled( true );
    ui->addChildElementButton->setEnabled( true );
    ui->commentLineEdit->setEnabled( true );
    ui->showCommentHelpButton->setEnabled( true );
    ui->emptyProfileHelpButton->setVisible( false );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::querySetActiveSession( QString reason )
{
  while( !GCDataBaseInterface::instance()->hasActiveSession() )
  {
    GCMessageSpace::showErrorMessageBox( this, reason );
    GCDBSessionManager *manager = createDBSessionManager();
    manager->selectActiveDatabase();
    delete manager;
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::readSettings()
{
  QSettings settings( ORGANISATION, APPLICATION );
  restoreGeometry( settings.value( "geometry" ).toByteArray() );
  restoreState( settings.value( "windowState" ).toByteArray() );

  if( settings.contains( "saveWindowInformation" ) )
  {
    ui->actionRememberWindowGeometry->setChecked( settings.value( "saveWindowInformation" ).toBool() );
  }

  if( settings.contains( "useDarkTheme" ) )
  {
    ui->actionUseDarkTheme->setChecked( settings.value( "useDarkTheme" ).toBool() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::saveSettings()
{
  QSettings settings( ORGANISATION, APPLICATION );

  if( ui->actionRememberWindowGeometry->isChecked() )
  {
    settings.setValue( "geometry", saveGeometry() );
    settings.setValue( "windowState", saveState() );
  }
  else
  {
    if( settings.contains( "geometry" ) )
    {
      settings.remove( "geometry" );
    }

    if( settings.contains( "windowState" ) )
    {
      settings.remove( "windowState" );
    }
  }

  settings.setValue( "saveWindowInformation", ui->actionRememberWindowGeometry->isChecked() );
  settings.setValue( "useDarkTheme", ui->actionUseDarkTheme->isChecked() );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::queryRestoreFiles()
{
  QDir dir = QDir::current();
  QStringList tempFiles = QDir::current()
                          .entryList( QDir::Files )
                          .filter( QString( "%1_temp" )
                                   .arg( GCDataBaseInterface::instance()->activeSessionName().remove( ".db" ) ) );

  if( !tempFiles.isEmpty() )
  {
    QMessageBox::StandardButton accept = QMessageBox::information( this,
                                                                   "Found recovery files",
                                                                   "Found auto-recover files related to this profile. Would you like to view and save?",
                                                                   QMessageBox::Ok | QMessageBox::Cancel,
                                                                   QMessageBox::Ok );

    if( accept == QMessageBox::Ok )
    {
      /* "Delete on close" flag set. */
      GCRestoreFilesForm *restore = new GCRestoreFilesForm( tempFiles, this );
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
    outStream << m_domDoc->toString( 2 );
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
