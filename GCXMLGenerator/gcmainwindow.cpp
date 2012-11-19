#include "gcmainwindow.h"
#include "ui_gcmainwindow.h"
#include "db/gcdatabaseinterface.h"
#include "xml/xmlsyntaxhighlighter.h"
#include "forms/gcnewelementform.h"
#include "forms/gcmessagedialog.h"
#include "utils/gccombobox.h"

#include <QSignalMapper>
#include <QDomDocument>
#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QComboBox>
#include <QTimer>
#include <QModelIndex>

/*--------------------------------------------------------------------------------------*/

const QString EMPTY( "---" );
const qint64  DOMLIMIT( 3145728 );    // 3MB


/*--------------------------- NON-MEMBER UTILITY FUNCTIONS ----------------------------*/

QString getScrollAnchorText( const QDomElement &element )
{
  QString anchor( "<" );
  anchor += element.tagName();

  QDomNamedNodeMap attributes = element.attributes();

  /* For elements with no children (e.g. <element/> */
  if( attributes.isEmpty() &&
      element.childNodes().isEmpty() )
  {
    anchor += "/>";
    return anchor;
  }

  if( !attributes.isEmpty() )
  {
    for( int i = 0; i < attributes.size(); ++i )
    {
      anchor += " ";

      QString attribute = attributes.item( i ).toAttr().name();
      anchor += attribute;
      anchor += "=\"";

      QString attributeValue = attributes.item( i ).toAttr().value();
      anchor += attributeValue;
      anchor += "\"";
    }

    anchor += "/>";
  }
  else
  {
    anchor += ">";
  }

  return anchor;
}

/*--------------------------------- MEMBER FUNCTIONS ----------------------------------*/

GCMainWindow::GCMainWindow( QWidget *parent ) :
  QMainWindow           ( parent ),
  ui                    ( new Ui::GCMainWindow ),
  m_dbInterface         ( new GCDataBaseInterface( this ) ),
  m_signalMapper        ( new QSignalMapper( this ) ),
  m_domDoc              ( NULL ),
  m_settings            ( NULL ),
  m_currentCombo        ( NULL ),
  m_saveTimer           ( NULL ),
  m_currentXMLFileName  ( "" ),
  m_activeAttributeName ( "" ),
  m_userCancelled       ( false ),
  m_superUserMode       ( false ),
  m_rootElementSet      ( false ),
  m_wasTreeItemActivated( false ),
  m_newElementWasAdded  ( false ),
  m_rememberPreference  ( false ),
  m_busyImporting       ( false ),
  m_DOMTooLarge         ( false ),
  m_treeItemNodes       (),
  m_comboBoxes          (),
  m_messages            ()
{
  ui->setupUi( this );

  /* Hide super user options. */
  ui->addAttributeButton->setVisible( false );
  ui->addAttributeLabel->setVisible( false );
  ui->addAttributeLineEdit->setVisible( false );
  ui->addNewElementPushButton->setVisible( false );
  ui->textSaveButton->setVisible( false );
  ui->textRevertButton->setVisible( false );
  ui->dockWidgetTextEdit->setReadOnly( true );

  /* The user must see these actions exist, but shouldn't be able to access
    them except in super user mode. */
  ui->actionAddNewDatabase->setEnabled( false );
  ui->actionRemoveDatabase->setEnabled( false );
  ui->actionImportXMLToDatabase->setEnabled( false );

  /* Database related. */
  connect( ui->actionAddNewDatabase,        SIGNAL( triggered() ),     this, SLOT( addNewDB() ) );
  connect( ui->actionAddExistingDatabase,   SIGNAL( triggered() ),     this, SLOT( addExistingDB() ) );
  connect( ui->actionRemoveDatabase,        SIGNAL( triggered() ),     this, SLOT( removeDB() ) );
  connect( ui->actionSwitchSessionDatabase, SIGNAL( triggered() ),     this, SLOT( switchDBSession() ) );
  connect( ui->actionImportXMLToDatabase,   SIGNAL( triggered() ),     this, SLOT( importXMLToDatabase() ) );

  /* XML File related. */
  connect( ui->actionNew,                   SIGNAL( triggered() ),     this, SLOT( newXMLFile() ) );
  connect( ui->actionOpen,                  SIGNAL( triggered() ),     this, SLOT( openXMLFile() ) );
  connect( ui->actionSave,                  SIGNAL( triggered() ),     this, SLOT( saveXMLFile() ) );
  connect( ui->actionSaveAs,                SIGNAL( triggered() ),     this, SLOT( saveXMLFileAs() ) );

  /* Build XML/Edit DOM. */
  connect( ui->deleteElementButton,         SIGNAL( clicked() ),       this, SLOT( deleteElementFromDOM() ) );
  connect( ui->addElementButton,            SIGNAL( clicked() ),       this, SLOT( addChildElementToDOM() ) );
  connect( ui->textSaveButton,              SIGNAL( clicked() ),       this, SLOT( saveDirectEdit() ) );
  connect( ui->textRevertButton,            SIGNAL( clicked() ),       this, SLOT( revertDirectEdit() ) );

  /* Various other actions. */
  connect( ui->actionSuperUserMode,         SIGNAL( toggled( bool ) ), this, SLOT( switchSuperUserMode( bool ) ) );
  connect( ui->expandAllCheckBox,           SIGNAL( clicked( bool ) ), this, SLOT( collapseOrExpandTreeWidget( bool ) ) );
  connect( ui->actionExit,                  SIGNAL( triggered() ),     this, SLOT( close() ) );
  connect( ui->addNewElementPushButton,     SIGNAL( clicked() ),       this, SLOT( showNewElementForm() ) );
  connect( ui->actionForgetPreferences,     SIGNAL( triggered() ),     this, SLOT( forgetAllMessagePreferences() ) );

  /* Everything tree widget related ("itemChanged" will only ever be emitted in Super User mode
    since tree widget items aren't editable otherwise). */
  connect( ui->treeWidget,                  SIGNAL( itemChanged  ( QTreeWidgetItem*, int ) ), this, SLOT( treeWidgetItemChanged  ( QTreeWidgetItem*, int ) ) );
  connect( ui->treeWidget,                  SIGNAL( itemClicked  ( QTreeWidgetItem*, int ) ), this, SLOT( treeWidgetItemActivated( QTreeWidgetItem*, int ) ) );
  connect( ui->treeWidget,                  SIGNAL( itemActivated( QTreeWidgetItem*, int ) ), this, SLOT( treeWidgetItemActivated( QTreeWidgetItem*, int ) ) );

  /* Everything table widget related ("itemChanged" will only ever be emitted in Super User mode
    since table widget items aren't editable otherwise). */
  connect( ui->tableWidget,                 SIGNAL( itemChanged  ( QTableWidgetItem* ) ),     this, SLOT( attributeNameChanged  ( QTableWidgetItem* ) ) );
  connect( ui->tableWidget,                 SIGNAL( itemClicked  ( QTableWidgetItem* ) ),     this, SLOT( setActiveAttributeName( QTableWidgetItem* ) ) );
  connect( ui->tableWidget,                 SIGNAL( itemActivated( QTableWidgetItem* ) ),     this, SLOT( setActiveAttributeName( QTableWidgetItem* ) ) );

  /* Initialise the database interface and retrieve the list of database names (this will
    include the path references to the ".db" files). */
  if( !m_dbInterface->initialise() )
  {
    showErrorMessageBox( m_dbInterface->getLastError() );
    this->close();
  }

  m_domDoc   = new QDomDocument;
  m_settings = new QSettings( "GoblinCoding", "XML Studio", this );
  m_settings->setValue( "Messages", "Save dialog prompt user preferences.");

  /* If the interface was successfully initialised, prompt the user to choose a database
    connection for this session. */
  showKnownDBForm( GCKnownDBForm::ShowAll );

  /* Everything happens automagically. */
  XmlSyntaxHighlighter *highLighter = new XmlSyntaxHighlighter( ui->dockWidgetTextEdit );
  Q_UNUSED( highLighter );

  connect( m_signalMapper, SIGNAL( mapped( QWidget* ) ), this, SLOT( setCurrentComboBox( QWidget* ) ) );
}

/*--------------------------------------------------------------------------------------*/

GCMainWindow::~GCMainWindow()
{
  // TODO: Give the user the option to save the current XML file.

  delete m_domDoc;
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::openXMLFile()
{
  if( !m_dbInterface->hasActiveSession() )
  {
    QString errMsg( "No active profile set, please set one for this session." );
    showErrorMessageBox( errMsg );
    showKnownDBForm( GCKnownDBForm::ShowAll );
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
      return;
    }

    /* Reset the DOM only after we've successfully opened the file. */
    resetDOM();

    QTextStream inStream( &file );
    QString fileContent( inStream.readAll() );
    m_DOMTooLarge = file.size() > DOMLIMIT;
    file.close();

    /* This application isn't optimised for dealing with very large XML files (the entire point is that
      this suite should provide the functionality necessary for the manual manipulation of, e.g. XML config
      files normally set up by hand via copy and paste exercises), if this file is too large to be handled
      comfortably, we need to let the user know and also make sure that we don't try to set the DOM content
      as text in the QTextEdit (QTextEdit is optimised for paragraphs). */
    if( m_DOMTooLarge )
    {
      bool remembered = m_settings->value( "Messages/Message07", false ).toBool();

      if( !remembered )
      {
        GCMessageDialog *dialog = new GCMessageDialog( "Large file!",
                                                       "The file you just opened is a bit too large for us "
                                                       "to handle comfortably.  Feel free to try working on it, but "
                                                       "you definitely won't be able to see your changes "
                                                       "in the text edit and things may also become impossibly slow.",
                                                       GCMessageDialog::OKOnly,
                                                       GCMessageDialog::OK,
                                                       GCMessageDialog::Information );

        connect( dialog, SIGNAL( rememberUserChoice( bool ) ), this, SLOT( rememberPreference( bool ) ) );
        dialog->exec();
        saveSetting( "Messages/Message07", true );
      }
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
      showErrorMessageBox( errorMsg );
      resetDOM();
      return;
    }

    /* If the user is opening an XML file of a kind that isn't supported by the current active session,
      we need to warn him/her of this fact and provide them with a couple of options (depending on which
      privileges the current user mode has). */
    if( !m_dbInterface->knownRootElements().contains( m_domDoc->documentElement().tagName() ) )
    {
      if( !m_superUserMode )
      {
        do
        {
          /* This message must always be shown (i.e. we don't have to show the custom
          dialog box that provides the \"Don't show this again\" option). */
          QMessageBox::warning( this,
                                "Unknown XML Style",
                                "The current active profile has no knowledge of the\n"
                                "specific XML style (the elements, attributes, attribute values and\n"
                                "all the associations between them) of the document you are trying to open.\n\n"
                                "You can either:\n\n"
                                "1. Select an existing profile that describes this type of XML, or\n"
                                "2. Switch to \"Super User\" mode and open the file again to import it to the profile." );

          showKnownDBForm( GCKnownDBForm::SelectAndExisting );

        } while( !m_dbInterface->knownRootElements().contains( m_domDoc->documentElement().tagName() ) &&
                 !m_userCancelled );

        /* If the user selected a database that fits, process the DOM, otherwise reset everything. */
        if( !m_userCancelled )
        {
          processDOMDoc();
        }
        else
        {
          resetDOM();
        }

        m_userCancelled = false;
      }
      else
      {
        /* If we're not already busy importing an XML file, check if the
        user maybe wants to do so. */
        if( !m_busyImporting )
        {
          /* Check if the user previously requested that his/her accept must be saved. */
          bool remembered = m_settings->value( "Messages/Message01", false ).toBool();

          if( !remembered )
          {
            /* If the user is a super user, he/she might want to import the XML profile to the
            current database. */
            GCMessageDialog *dialog = new GCMessageDialog( "Import XML?",
                                                           "Would you like to import the XML document to the active profile?",
                                                           GCMessageDialog::YesNo,
                                                           GCMessageDialog::No,
                                                           GCMessageDialog::Question );

            connect( dialog, SIGNAL( rememberUserChoice( bool ) ), this, SLOT( rememberPreference( bool ) ) );

            QDialog::DialogCode accept = static_cast< QDialog::DialogCode >( dialog->exec() );

            if( accept == QDialog::Accepted )
            {
              importXMLToDatabase();
              saveSetting( "Messages/Message01", true );
              saveSetting( "Messages/Message01/Preference", true );
            }
            else
            {
              saveSetting( "Messages/Message01", true );
              saveSetting( "Messages/Message01/Preference", false );
            }
          }
          else
          {
            /* If we do have a remembered setting, act accordingly. */
            bool batchProcess = m_settings->value( "Messages/Message01/Preference" ).toBool();

            if( batchProcess )
            {
              importXMLToDatabase();
            }
          }
        }
        else
        {
          /* If we're already busy importing, it means the user explicitly requested
          an XML import, didn't have a current document active and confirmed that he/she
          wanted to open an XML file to import.  Furthermore, there is no risk of an
          endless loop since the DOM document will have been populated by the time we
          get to this point, which will ensure that only the first part of the following
          function's logic will be executed..."openXMLFile" won't be called again. */
          importXMLToDatabase();
        }
      }
    }
    else
    {
      /* If the user selected a database that knows of this particular XML profile,
        simply process the document. */
      processDOMDoc();
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::newXMLFile()
{
  resetDOM();

  ui->addElementComboBox->clear();
  ui->addElementComboBox->addItems( m_dbInterface->knownRootElements() );

  toggleAddElementWidgets();

  ui->actionSave->setEnabled( true );
  ui->actionSaveAs->setEnabled( true );

  m_rootElementSet = false;
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
    startSaveTimer();
    m_currentXMLFileName = file;
    saveXMLFile();
  }
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

void GCMainWindow::resetDOM()
{
  m_currentXMLFileName = "";
  m_domDoc->clear();  
  m_treeItemNodes.clear();
  ui->treeWidget->clear();
  ui->dockWidgetTextEdit->clear();
  resetTableWidget();  

  /* The timer will be reactivated as soon as work starts again on a legitimate
    document and the user saves it for the first time. */
  if( m_saveTimer )
  {
    m_saveTimer->stop();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::processDOMDoc()
{
  ui->treeWidget->clear(); // also deletes current items
  m_treeItemNodes.clear();
  resetTableWidget();

  QDomElement root = m_domDoc->documentElement();

  /* We want to show the document's root element in the tree widget as well. */
  QTreeWidgetItem *item = new QTreeWidgetItem;
  item->setText( 0, root.tagName() );

  if( m_superUserMode )
  {
    item->setFlags( item->flags() | Qt::ItemIsEditable );
  }

  ui->treeWidget->invisibleRootItem()->addChild( item );  // takes ownership
  m_treeItemNodes.insert( item, root );

  /* Now we can recursively stick the rest of the elements into the tree widget. */
  populateTreeWidget( root, item );

  /* Enable file save options. */
  ui->actionSave->setEnabled( true );
  ui->actionSaveAs->setEnabled( true );

  /* Display the DOM content in the text edit. */
  setTextEditXML( QDomElement() );

  /* If the user just added the root element, we need to make sure that they don't
    try to add it again...it happens. */
  if( !m_rootElementSet )
  {
    ui->treeWidget->setCurrentItem( item, 0 );
    treeWidgetItemActivated( item, 0 );
    m_rootElementSet = true;
  }

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

    if( m_superUserMode )
    {
      item->setFlags( item->flags() | Qt::ItemIsEditable );
    }

    parentItem->addChild( item );  // takes ownership
    m_treeItemNodes.insert( item, element );

    populateTreeWidget( element, item );
    element = element.nextSiblingElement();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::treeWidgetItemChanged( QTreeWidgetItem *item, int column )
{
  /* This slot will only be called in Super User mode so we can safely keep the functionality
    as it is (i.e. updating the database alongside the DOM) without any other explicit checks. */
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
          be available to the map as well) and the tree widget. */
        QDomNodeList list = m_domDoc->elementsByTagName( previousName );

        for( int i = 0; i < list.count(); ++i )
        {
          QDomElement element( list.at( i ).toElement() );
          element.setTagName( elementName );
          const_cast< QTreeWidgetItem* >( m_treeItemNodes.key( element ) )->setText( column, elementName );
        }

        /* The name change may introduce a new element name to the DB, we can safely call
          "addElement" below as it doesn't do anything if the element already exists in the database. */
        bool success( false );
        QStringList attributes = m_dbInterface->attributes( previousName, success );

        m_dbInterface->addElement( elementName,
                                   m_dbInterface->children( previousName, success ),
                                   attributes );

        foreach( QString attribute, attributes )
        {
          m_dbInterface->updateAttributeValues( elementName,
                                                attribute,
                                                m_dbInterface->attributeValues( previousName, attribute, success ) );
        }

        if( !success )
        {
          showErrorMessageBox( m_dbInterface->getLastError() );
        }

        setTextEditXML( m_treeItemNodes.value( item ).toElement() );
      }
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::treeWidgetItemActivated( QTreeWidgetItem *item, int column )
{
  Q_UNUSED( column );

  /* Because the table widget is re-populated with the attribute names and
    values associated with the activated tree widget item, this flag is set
    to prevent the functionality in "attributeNameChanged" (which is triggered
    by the population of the table widget). */
  m_wasTreeItemActivated = true;

  resetTableWidget();

  /* Get only the attributes currently assigned to the element
    corresponding to the activated item (and the lists of associated
    values for these attributes) and populate our table widget. */
  bool success( false );
  QDomElement element = m_treeItemNodes.value( item );
  QStringList attributeNames = m_dbInterface->attributes( element.tagName(), success );

  /* This is more for debugging than for end-user functionality. */
  if( !success )
  {
    showErrorMessageBox( m_dbInterface->getLastError() );
  }

  /* Add all the known attribute names in the cells in the first column
    of the table widget, create and populate combo boxes with the values
    associated with the attribute in question and insert the combo boxes
    into the second column of the table widget.  Attribute names are only
    editable in Super User mode whereas attribute values can always be edited.
    Finally we insert an "empty" row when in Super User mode so that the user
    may add additional attributes and values to the current element. */
  for( int i = 0; i < attributeNames.count(); ++i )
  {
    QTableWidgetItem *label = new QTableWidgetItem( attributeNames.at( i ) );

    /* Items are editable by default, disable this option if not in Super User mode. */
    if( !m_superUserMode )
    {
      label->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable );
    }

    ui->tableWidget->setRowCount( i + 1 );
    ui->tableWidget->setItem( i, 0, label );

    GCComboBox *attributeCombo = new GCComboBox;
    attributeCombo->addItems( m_dbInterface->attributeValues( element.tagName(), attributeNames.at( i ), success ) );
    attributeCombo->insertItem( 0, EMPTY );
    attributeCombo->setEditable( true );

    /* This is more for debugging than for end-user functionality. */
    if( !success )
    {
      showErrorMessageBox( m_dbInterface->getLastError() );
    }

    /* If we are still in the process of building the document, the attribute value will
      be empty since it has never been set before.  For this particular case,
      calling "findText" will result in a null pointer exception. */
    QString attributeValue = element.attribute( attributeNames.at( i ) );

    if( !attributeValue.isEmpty() )
    {
      attributeCombo->setCurrentIndex( attributeCombo->findText( attributeValue ) );
    }
    else
    {
      attributeCombo->setCurrentIndex( 0 );
    }

    /* Attempting the connection before we've set the current index causes the
      "attributeValueChanged" slot to be called too early, resulting in a segmentation
      fault due to value conflicts/missing values (i.e we can't do the connect before
      we've set the current index). */
    connect( attributeCombo, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( attributeValueChanged  ( QString ) ) );

    ui->tableWidget->setCellWidget( i, 1, attributeCombo );
    m_comboBoxes.insert( attributeCombo, i );

    /* This will point the current combo box member to the combo that's been activated
      in the table widget (used in "attributeValueChanged" to obtain the row number the
      combo box appears in in the table widget, etc, etc). */
    connect( attributeCombo, SIGNAL( activated( int ) ), m_signalMapper, SLOT( map() ) );
    m_signalMapper->setMapping( attributeCombo, attributeCombo );
  }

  /* Add the "empty" row as described above when in Super User mode. */
  if( m_superUserMode )
  {
    QTableWidgetItem *label = new QTableWidgetItem( EMPTY );

    int lastRow = ui->tableWidget->rowCount();
    ui->tableWidget->setRowCount( lastRow + 1 );
    ui->tableWidget->setItem( lastRow, 0, label );

    /* Create the combo box, but deactivate it until we have an associated attribute name. */
    GCComboBox *attributeCombo = new GCComboBox;    
    attributeCombo->insertItem( 0, EMPTY );
    attributeCombo->setEnabled( false );

    connect( attributeCombo, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( attributeValueChanged  ( QString ) ) );

    ui->tableWidget->setCellWidget( lastRow, 1, attributeCombo );
    m_comboBoxes.insert( attributeCombo, lastRow );

    /* This will point the current combo box member to the combo that's been activated
      in the table widget (used in "attributeValueChanged" to obtain the row number the
      combo box appears in in the table widget, etc, etc). */
    connect( attributeCombo, SIGNAL( activated( int ) ), m_signalMapper, SLOT( map() ) );
    m_signalMapper->setMapping( attributeCombo, attributeCombo );
  }

  /* Populate the "add element" combo box with the known first level children of the
    highlighted element. */
  ui->addElementComboBox->clear();
  ui->addElementComboBox->addItems( m_dbInterface->children( element.tagName(), success ) );
  toggleAddElementWidgets();

  /* This is more for debugging than for end-user functionality. */
  if( !success )
  {
    showErrorMessageBox( m_dbInterface->getLastError() );
  }

  ui->dockWidgetTextEdit->moveCursor( QTextCursor::Start );
  ui->dockWidgetTextEdit->find( getScrollAnchorText( element ) );
  ui->dockWidgetTextEdit->ensureCursorVisible();

  /* Unset flag. */
  m_wasTreeItemActivated = false;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setCurrentComboBox( QWidget *combo )
{
  m_currentCombo = combo;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setActiveAttributeName( QTableWidgetItem *item )
{
  m_activeAttributeName = item->text();
}

/*--------------------------------------------------------------------------------------*/

/* This slot will only ever be called in Super User mode. */
void GCMainWindow::attributeNameChanged( QTableWidgetItem *item )
{
  /* Don't execute the logic if a tree widget item's activation is triggering
    a re-population of the table widget, resulting in this slot being called. */
  if( !m_wasTreeItemActivated )
  {
    /* All attribute name changes will be assumed to be additions, removing an attribute
      with a specific name has to be done explicitly. */
    QTreeWidgetItem *currentItem = ui->treeWidget->currentItem();
    QDomElement currentElement = m_treeItemNodes.value( currentItem );

    if( !m_dbInterface->updateElementAttributes( currentElement.tagName(), QStringList( item->text() ) ) )
    {
      showErrorMessageBox( m_dbInterface->getLastError() );
    }
    else
    {
      /* The current attribute value will be displayed in the second column (the
        combo box next to the currently selected table widget item). */
      GCComboBox *attributeValueCombo = dynamic_cast< GCComboBox* >( ui->tableWidget->cellWidget( ui->tableWidget->currentRow(), 1 ) );

      if( attributeValueCombo )
      {
        if( attributeValueCombo->currentText() != EMPTY )
        {
          currentElement.removeAttribute( m_activeAttributeName );
          currentElement.setAttribute( item->text(), attributeValueCombo->currentText() );
        }
        else
        {          
          currentElement.setAttribute( item->text(), "" );

          /* If the attribute value was empty, we might have just started
            editing a previously inactive row (in other words this could
            be the first time that an attribute of this name has been created).
            Enable the attribute value combo box in this case. */
          if( m_activeAttributeName == EMPTY &&
              !attributeValueCombo->isEnabled() )
          {
            attributeValueCombo->setEnabled( true );
          }
        }

        bool success;
        QStringList attributeValues = m_dbInterface->attributeValues( currentElement.tagName(),
                                                                      m_activeAttributeName,
                                                                      success );

        if( !m_dbInterface->updateAttributeValues( currentElement.tagName(),
                                                   item->text(),
                                                   attributeValues ) ||
            !success)
        {
          showErrorMessageBox( m_dbInterface->getLastError() );
        }
      }

      setTextEditXML( currentElement );

      /* If the user added a new attribute, we wish to insert another new
        "empty" row so that he/she may add even more attributes if he/she
        wishes to do so. We also need to update the active attribute name
        to the new attribute name (normally this is handled by a signal
        that calls "setActiveAttributeName" but these signals are emitted
        by clicking on the table widget only). */
      if( m_activeAttributeName == EMPTY )
      {
        treeWidgetItemActivated( ui->treeWidget->currentItem(), 0 );        
      }
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::attributeValueChanged( const QString &value )
{
  /* Don't execute the logic if a tree widget item's activation is triggering
    a re-population of the table widget, resulting in this slot being called. */
  if( !m_wasTreeItemActivated )
  {
    QTreeWidgetItem *currentItem = ui->treeWidget->currentItem();
    QDomElement currentElement = m_treeItemNodes.value( currentItem );

    /* The current attribute will be displayed in the first column (next to the
    combo box which will be the actual current item). */
    QString currentAttributeName = ui->tableWidget->item( m_comboBoxes.value( m_currentCombo ), 0 )->text();

    /* If the user sets the attribute value to EMPTY, the attribute is removed from
    the current document. */
    if( value == EMPTY )
    {
      currentElement.removeAttribute( currentAttributeName );
    }
    else
    {
      currentElement.setAttribute( currentAttributeName, value );

      /* If we don't know about this value, we need to add it to the DB. */
      bool success( false );
      QStringList attributeValues = m_dbInterface->attributeValues( currentElement.tagName(), currentAttributeName, success );

      if( success )
      {
        if( !attributeValues.contains( value ) )
        {
          if( !m_dbInterface->updateAttributeValues( currentElement.tagName(),
                                                     currentAttributeName,
                                                     QStringList( value ) ) )
          {
            showErrorMessageBox( m_dbInterface->getLastError() );
          }
        }
      }
      else
      {
        showErrorMessageBox( m_dbInterface->getLastError() );
      }
    }

    setTextEditXML( currentElement );
  }
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

void GCMainWindow::deleteElementFromDOM()
{
  QTreeWidgetItem *currentItem = ui->treeWidget->currentItem();
  QDomElement currentElement = m_treeItemNodes.value( currentItem );

  /* Remove the element from the DOM first. */
  QDomNode parentNode = currentElement.parentNode();
  parentNode.removeChild( currentElement );

  /* Now we can whack it from the tree widget and map. */
  m_treeItemNodes.remove( currentItem );

  QTreeWidgetItem *parentItem = currentItem->parent();
  parentItem->removeChild( currentItem );

  setTextEditXML( parentNode.toElement() );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addChildElementToDOM()
{
  QString newElementName = ui->addElementComboBox->currentText();

  if( !newElementName.isEmpty() )
  {
    /* Update the tree widget. */
    QTreeWidgetItem *newItem = new QTreeWidgetItem;
    newItem->setText( 0, newElementName );

    if( m_superUserMode )
    {
      newItem->setFlags( newItem->flags() | Qt::ItemIsEditable );
    }

    /* Update the current DOM document. */
    QDomElement newElement = m_domDoc->createElement( newElementName );

    if( !m_treeItemNodes.isEmpty() )
    {
      QTreeWidgetItem *currentItem = ui->treeWidget->currentItem();
      currentItem->addChild( newItem );

      /* Expand the item's parent for convenience. */
      ui->treeWidget->expandItem( currentItem );

      QDomElement parent = m_treeItemNodes.value( currentItem );
      parent.appendChild( newElement );

      /* If "addChildElementToDOM" was called from within "addNewElement", then
        the new element name must be added as a child of the current element. */
      if( m_newElementWasAdded )
      {
        m_dbInterface->updateElementChildren( currentItem->text( 0 ), QStringList( newElementName ) );
      }
    }
    else
    {
      /* If the user starts creating a DOM document without having explicitly asked for
      a new file to be created, do it automatically (we can't call "newXMLFile here" since
      it resets the DOM document as well). */
      m_currentXMLFileName = "";
      ui->actionSave->setEnabled( true );
      ui->actionSaveAs->setEnabled( true );

      ui->treeWidget->invisibleRootItem()->addChild( newItem );  // takes ownership
      m_domDoc->appendChild( newElement );

      /* If "addChildElementToDOM" was called from within "addNewElement", then
        the new element name will be a new root element. */
      if( m_newElementWasAdded )
      {
        m_dbInterface->addRootElement( newElementName );
      }
    }

    /* Keep everything in sync in the map. */
    m_treeItemNodes.insert( newItem, newElement );

    /* Add the known attributes associated with this element. */
    bool success( false );
    QStringList attributes = m_dbInterface->attributes( newElementName, success );

    if( success )
    {
      for( int i = 0; i < attributes.size(); ++i )
      {
        newElement.setAttribute( attributes.at( i ), QString( "" ) );
      }
    }
    else
    {
      showErrorMessageBox( m_dbInterface->getLastError() );
    }

    /* If we've just added a new element in Super User mode, we wish to set the current
      active tree item as the parent of the new element and not the new element itself.
      Failing to do so will cause a cascading effect where each new element added through
      the form will be the child of the previously added element.  We don't want this to
      happen as all newly added elements must be siblings. */
    if( !m_newElementWasAdded )
    {
      ui->treeWidget->setCurrentItem( newItem, 0 );
    }
    else
    {
      ui->treeWidget->setCurrentItem( newItem->parent(), 0 );
    }

    setTextEditXML( newElement );    

    /* If the user just added the root element, we need to make sure that they don't
    try to add it again...it happens. */
    if( !m_rootElementSet )
    {      
      treeWidgetItemActivated( newItem, 0 );
      m_rootElementSet = true;
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addNewElement( const QString &element, const QStringList &attributes )
{
  if( !element.isEmpty() )
  {
    /* Add the new element and associated attributes to the database. */
    m_dbInterface->addElement( element, QStringList(), attributes );

    /* The new element is added as a first level child of the current element (represented
      by the highlighted item in the tree view) so now we can update the DOM doc as well. */
    ui->addElementComboBox->insertItem( 0, element );
    ui->addElementComboBox->setCurrentIndex( 0 );

    m_newElementWasAdded = true;
    addChildElementToDOM();
    m_newElementWasAdded = false;
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addNewDB()
{
  QString file = QFileDialog::getSaveFileName( this, "Add New Profile", QDir::homePath(), "XML Profiles (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    addDBConnection( file );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addExistingDB()
{
  QString file = QFileDialog::getOpenFileName( this, "Add Existing Profile", QDir::homePath(), "XML Profiles (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    addDBConnection( file );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addDBConnection( const QString &dbName )
{
  if( !m_dbInterface->addDatabase( dbName ) )
  {
    showErrorMessageBox( m_dbInterface->getLastError() );
    return;
  }

  /* Check if we have a saved user preference for this situation. */
  bool remembered = m_settings->value( "Messages/Message02", false ).toBool();

  if( !remembered )
  {
    GCMessageDialog *dialog = new GCMessageDialog( "Set Session",
                                                   "Would you like to set the new profile as active?",
                                                   GCMessageDialog::YesNo,
                                                   GCMessageDialog::Yes,
                                                   GCMessageDialog::Question );

    connect( dialog, SIGNAL( rememberUserChoice( bool ) ), this, SLOT( rememberPreference( bool ) ) );

    QDialog::DialogCode accept = static_cast< QDialog::DialogCode >( dialog->exec() );

    if( accept == QDialog::Accepted )
    {
      saveSetting( "Messages/Message02", true );
      saveSetting( "Messages/Message02/Preference", true );

      setSessionDB( dbName );
    }
    else
    {
      saveSetting( "Messages/Message02", true );
      saveSetting( "Messages/Message02/Preference", false );

      if( !m_dbInterface->hasActiveSession() )
      {
        showKnownDBForm( GCKnownDBForm::ShowAll );
      }
    }
  }
  else
  {
    bool setSession = m_settings->value( "Messages/Message02/Preference" ).toBool();

    if( setSession )
    {
      setSessionDB( dbName );
    }
    else
    {
      if( !m_dbInterface->hasActiveSession() )
      {
        showKnownDBForm( GCKnownDBForm::ShowAll );
      }
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setSessionDB( const QString &dbName )
{
  if( !m_dbInterface->setSessionDB( dbName ) )
  {
    QString error = QString( "Failed to set session \"%1\" as active - [%2]" ).arg( dbName )
                    .arg( m_dbInterface->getLastError() );
    showErrorMessageBox( error );
  }
  else
  {
    /* If the user set an empty database, prompt to populate it.  This message must
      always be shown (i.e. we don't have to show the custom dialog box that provides
      the \"Don't show this again\" option). */
    if( m_dbInterface->knownElements().size() < 1 )
    {
      QMessageBox::warning( this,
                            "Empty Profile",
                            "The current active profile is completely empty (aka \"entirely useless\").\n"
                            "You can either:\n"
                            "1. Select a different (populated) profile and continue working, or\n"
                            "2. Switch to \"Super User\" mode and start populating this one." );
    }

    /* If we have an empty DOM doc, load the list of known document root elements
      to start the document building process. */
    if( m_domDoc->documentElement().isNull() )
    {
      ui->addElementComboBox->clear();
      ui->addElementComboBox->addItems( m_dbInterface->knownRootElements() );
      toggleAddElementWidgets();
      m_rootElementSet = false;
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::removeDB()
{
  showKnownDBForm( GCKnownDBForm::ToRemove );
}

/*--------------------------------------------------------------------------------------*/


void GCMainWindow::removeDBConnection( const QString &dbName )
{
  if( !m_dbInterface->removeDatabase( dbName ) )
  {
    QString error = QString( "Failed to remove profile \"%1\": [%2]" ).arg( dbName )
                    .arg( m_dbInterface->getLastError() );
    showErrorMessageBox( error );
  }

  /* If the user removed the active DB for this session, we need to know
    what he/she intends to replace it with. */
  if( !m_dbInterface->hasActiveSession() )
  {
    QString errMsg( "The active profile has been removed, please set another as active." );
    showErrorMessageBox( errMsg );
    showKnownDBForm( GCKnownDBForm::ShowAll );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::switchDBSession()
{
  /* Switching DB sessions while building an XML document could result in all kinds of trouble
    since the items known to the current session may not be known to the next. */
  if( !m_treeItemNodes.isEmpty() )
  {
    /* Check if we have a previously saved user preference. */
    bool remembered = m_settings->value( "Messages/Message03", false ).toBool();

    if( !remembered )
    {
      GCMessageDialog *dialog = new GCMessageDialog( "Warning!",
                                                     "Switching profile sessions while building an XML document "
                                                     "will cause the document to be reset and your work will be lost. "
                                                     "If this is fine, proceed with \"OK\".\n\n"
                                                     "On the other hand, if you wish to keep your work, please hit \"Cancel\" and "
                                                     "save the document first before coming back here.",
                                                     GCMessageDialog::OKCancel,
                                                     GCMessageDialog::Cancel,
                                                     GCMessageDialog::Warning );

      connect( dialog, SIGNAL( rememberUserChoice( bool ) ), this, SLOT( rememberPreference( bool ) ) );

      QDialog::DialogCode accept = static_cast< QDialog::DialogCode >( dialog->exec() );

      if( accept == QDialog::Accepted )
      {
        resetDOM();
        saveSetting( "Messages/Message03", true );
        saveSetting( "Messages/Message03/Preference", true );
      }
      else
      {
        saveSetting( "Messages/Message03", true );
        saveSetting( "Messages/Message03/Preference", false );

        return;
      }
    }
    else
    {
      bool reset = m_settings->value( "Messages/Message03/Preference" ).toBool();

      if( reset )
      {
        resetDOM();
      }
      else
      {
        return;
      }
    }
  }

  showKnownDBForm( GCKnownDBForm::ShowAll );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::importXMLToDatabase()
{
  m_busyImporting = true;

  /* This slot will only ever be called in Super User mode. */
  if (!m_domDoc->documentElement().isNull() )
  {
    /* Update the DB in one go. */
    if( !m_dbInterface->batchProcessDOMDocument( m_domDoc ) )
    {
      showErrorMessageBox( m_dbInterface->getLastError() );
    }
    else
    {
      processDOMDoc();
    }
  }
  else
  {
    bool remembered = m_settings->value( "Messages/Message06", false ).toBool();

    if( !remembered )
    {
      GCMessageDialog *dialog = new GCMessageDialog( "No active document",
                                                     "There is no document currently active, "
                                                     "would you like to open a document from file?",
                                                     GCMessageDialog::YesNo,
                                                     GCMessageDialog::Yes,
                                                     GCMessageDialog::Question );

      connect( dialog, SIGNAL( rememberUserChoice( bool ) ), SLOT( rememberPreference( bool ) ) );

      QDialog::DialogCode accept = static_cast< QDialog::DialogCode >( dialog->exec() );

      if( accept )
      {
        saveSetting( "Messages/Message06", true );
        saveSetting( "Messages/Message06/Preference", true );
        openXMLFile();
      }
      else
      {
        saveSetting( "Messages/Message06", true );
        saveSetting( "Messages/Message06/Preference", false );
      }
    }
    else
    {
      bool openFile = m_settings->value( "Messages/Message06/Preference" ).toBool();

      if( openFile )
      {
        openXMLFile();
      }
    }
  }

  m_busyImporting = false;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::deleteElementFromDB()
{

}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::deleteAttributeValuesFromDB()
{

}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::revertDirectEdit()
{
  setTextEditXML( QDomElement() );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::saveDirectEdit()
{
  /* This slot will only ever be called in Super User mode. */
  QString xmlErr( "" );
  int     line  ( -1 );
  int     col   ( -1 );

  if( !m_domDoc->setContent( ui->dockWidgetTextEdit->toPlainText(), &xmlErr, &line, &col ) )
  {
    QString errorMsg = QString( "XML is broken - Error [%1], line [%2], column [%3]" )
        .arg( xmlErr )
        .arg( line )
        .arg( col );
    showErrorMessageBox( errorMsg );
    resetDOM();
    return;
  }
  else
  {
    importXMLToDatabase();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showNewElementForm()
{
  /* Check if there is a previously saved user preference for this action. */
  bool remembered = m_settings->value( "Messages/Message04", false ).toBool();

  if( !remembered )
  {
    GCMessageDialog *dialog = new GCMessageDialog( "Careful!",
                                                   "All the new elements you add will be added as first level "
                                                   "children of the element highlighted in the tree view (in "
                                                   "other words it will become a sibling to the elements currently "
                                                   "in the dropdown menu).\n\n"
                                                   "If this is not what you intended, I suggest you \"Cancel\".",
                                                   GCMessageDialog::OKCancel,
                                                   GCMessageDialog::OK,
                                                   GCMessageDialog::Warning );

    connect( dialog, SIGNAL( rememberUserChoice( bool ) ), this, SLOT( rememberPreference( bool ) ) );

    QDialog::DialogCode accept = static_cast< QDialog::DialogCode >( dialog->exec() );

    if( accept == QDialog::Accepted )
    {
      GCNewElementForm *form = new GCNewElementForm;
      form->setWindowModality( Qt::ApplicationModal );
      connect( form, SIGNAL( newElementDetails( QString,QStringList ) ), this, SLOT( addNewElement( QString,QStringList ) ) );
      form->show();

      saveSetting( "Messages/Message04", true );
    }
    else
    {
      /* We don't want to remember a \"Cancel\" option for this particular situation. */
      saveSetting( "Messages/Message04", false );
    }
  }
  else
  {
    GCNewElementForm *form = new GCNewElementForm;
    form->setWindowModality( Qt::ApplicationModal );
    connect( form, SIGNAL( newElementDetails( QString,QStringList ) ), this, SLOT( addNewElement( QString,QStringList ) ) );
    form->show();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showKnownDBForm( GCKnownDBForm::Buttons buttons )
{
  GCKnownDBForm *knownDBForm = new GCKnownDBForm( m_dbInterface->getDBList(), buttons, this );

  connect( knownDBForm,   SIGNAL( newConnection() ),       this, SLOT( addNewDB() ) );
  connect( knownDBForm,   SIGNAL( existingConnection() ),  this, SLOT( addExistingDB() ) );
  connect( knownDBForm,   SIGNAL( dbSelected( QString ) ), this, SLOT( setSessionDB( QString ) ) );
  connect( knownDBForm,   SIGNAL( dbRemoved ( QString ) ), this, SLOT( removeDBConnection( QString ) ) );

  /* If we don't have an active DB session, it's probably at program
    start-up and the user wishes to exit the application by clicking "Cancel". */
  if( !m_dbInterface->hasActiveSession() )
  {
    connect( knownDBForm, SIGNAL( userCancelled() ), this, SLOT( close() ) );
    knownDBForm->show();
  }
  else
  {
    connect( knownDBForm, SIGNAL( userCancelled() ), this, SLOT( userCancelledKnownDBForm() ) );
    knownDBForm->exec();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::userCancelledKnownDBForm()
{
  m_userCancelled = true;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::switchSuperUserMode( bool super )
{
  m_superUserMode = super;

  if( m_superUserMode )
  {
    /* See if the user wants to see this message again. */
    bool remembered = m_settings->value( "Messages/Message05", false ).toBool();

    if( !remembered )
    {
      GCMessageDialog *dialog = new GCMessageDialog( "Super User Mode!",
                                                     "Absolutely everything you do in this mode is persisted to the "
                                                     "active profile and cannot be undone.\n\n"
                                                     "In other words, if anything goes wrong, it's all your fault...",
                                                     GCMessageDialog::OKOnly,
                                                     GCMessageDialog::OK,
                                                     GCMessageDialog::Warning );

      connect( dialog, SIGNAL( rememberUserChoice( bool ) ), this, SLOT( rememberPreference( bool ) ) );

      QDialog::DialogCode accept = static_cast< QDialog::DialogCode >( dialog->exec() );

      if( accept == QDialog::Accepted )
      {
        saveSetting( "Messages/Message05", true );
      }
    }
  }

  if( !m_dbInterface->hasActiveSession() )
  {
    showKnownDBForm( GCKnownDBForm::SelectAndExisting );
  }

  /* Set the new element and attribute options' visibility. */
  ui->addNewElementPushButton->setVisible( m_superUserMode );
  ui->addAttributeButton->setVisible( m_superUserMode );
  ui->addAttributeLabel->setVisible( m_superUserMode );
  ui->addAttributeLineEdit->setVisible( m_superUserMode );
  ui->textSaveButton->setVisible( m_superUserMode );
  ui->textRevertButton->setVisible( m_superUserMode );
  ui->dockWidgetTextEdit->setReadOnly( !m_superUserMode );

  /* The user must see these actions exist, but shouldn't be able to access
    them except when in "Super User" mode. */
  ui->actionAddNewDatabase->setEnabled( m_superUserMode );
  ui->actionRemoveDatabase->setEnabled( m_superUserMode );
  ui->actionImportXMLToDatabase->setEnabled( m_superUserMode );

  /* Needed to reset all the tree widget item's "editable" flags
    to whatever the current mode allows. */
  QList< QTreeWidgetItem* > itemList = m_treeItemNodes.keys();

  if( m_superUserMode )
  {
    for( int i = 0; i < itemList.size(); ++i )
    {
      const_cast< QTreeWidgetItem* >( itemList.at( i ) )->setFlags( Qt::ItemIsEnabled |
                                                                    Qt::ItemIsSelectable |
                                                                    Qt::ItemIsUserCheckable );
    }
  }
  else
  {
    for( int i = 0; i < itemList.size(); ++i )
    {
      QTreeWidgetItem *item = const_cast< QTreeWidgetItem* >( itemList.at( i ) );
      item->setFlags( item->flags() | Qt::ItemIsEditable );
    }
  }

  /* Reactivate the current item to populate the table widget with the new
    editable (or otherwise) combo boxes and attribute cells. */
  if( ui->treeWidget->currentItem() )
  {
    treeWidgetItemActivated( ui->treeWidget->currentItem(), 0 );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::rememberPreference( bool remember )
{
  m_rememberPreference = remember;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::forgetAllMessagePreferences()
{
  m_settings->beginGroup( "Messages" );
  m_settings->remove( "" );
  m_settings->endGroup();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::saveSetting( const QString &key, const QVariant &value )
{
  if( m_rememberPreference )
  {
    m_settings->setValue( key, value );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setTextEditXML( const QDomElement &element )
{
  if( !m_DOMTooLarge )
  {
    ui->dockWidgetTextEdit->setPlainText( m_domDoc->toString( 2 ) );

    if( !element.isNull() )
    {
      ui->dockWidgetTextEdit->find( getScrollAnchorText( element ) );
      ui->dockWidgetTextEdit->ensureCursorVisible();
    }
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

void GCMainWindow::toggleAddElementWidgets()
{
  /* Make sure we don't inadvertently create "empty" elements. */
  if( ui->addElementComboBox->count() < 1 )
  {
    ui->addElementComboBox->setEnabled( false );
    ui->addElementButton->setEnabled( false );
  }
  else
  {
    ui->addElementComboBox->setEnabled( true );
    ui->addElementButton->setEnabled( true );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showErrorMessageBox( const QString &errorMsg )
{
  QMessageBox::critical( this, "Error!", errorMsg );
}

/*--------------------------------------------------------------------------------------*/
