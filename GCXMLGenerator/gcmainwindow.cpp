#include "gcmainwindow.h"
#include "ui_gcmainwindow.h"
#include "db/gcdatabaseinterface.h"
#include "xml/xmlsyntaxhighlighter.h"
#include <QSignalMapper>
#include <QDomDocument>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QComboBox>
#include <QTimer>
#include <QModelIndex>

/*--------------------------------------------------------------------------------------*/

GCMainWindow::GCMainWindow( QWidget *parent ) :
  QMainWindow           ( parent ),
  ui                    ( new Ui::GCMainWindow ),
  m_dbInterface         ( new GCDataBaseInterface( this ) ),
  m_signalMapper        ( new QSignalMapper( this ) ),
  m_domDoc              ( NULL ),
  m_currentCombo        ( NULL ),
  m_saveTimer           ( NULL ),
  m_currentXMLFileName  ( "" ),
  m_activeAttributeName( "" ),
  m_userCancelled       ( false ),
  m_superUserMode       ( false ),
  m_rootElementSet      ( false ),
  m_wasTreeItemActivated( false ),
  m_treeItemNodes       (),
  m_comboBoxes          ()
{
  ui->setupUi( this );

  /* Hide super user options. */
  ui->addAttributeButton->setVisible( false );
  ui->addAttributeLabel->setVisible ( false );
  ui->addAttributeLineEdit->setVisible( false );

  /* The user must see these actions exist, but shouldn't be able to access
    them except in super user mode. */
  ui->actionAddNewDatabase->setEnabled( false );
  ui->actionRemoveDatabase->setEnabled( false );

  /* Database related. */
  connect( ui->actionAddNewDatabase,        SIGNAL( triggered() ),     this, SLOT( addNewDB() ) );
  connect( ui->actionAddExistingDatabase,   SIGNAL( triggered() ),     this, SLOT( addExistingDB() ) );
  connect( ui->actionRemoveDatabase,        SIGNAL( triggered() ),     this, SLOT( removeDB() ) );
  connect( ui->actionSwitchSessionDatabase, SIGNAL( triggered() ),     this, SLOT( switchDBSession() ) );

  /* XML File related. */
  connect( ui->actionNew,                   SIGNAL( triggered() ),     this, SLOT( newXMLFile() ) );
  connect( ui->actionOpen,                  SIGNAL( triggered() ),     this, SLOT( openXMLFile() ) );
  connect( ui->actionSave,                  SIGNAL( triggered() ),     this, SLOT( saveXMLFile() ) );
  connect( ui->actionSaveAs,                SIGNAL( triggered() ),     this, SLOT( saveXMLFileAs() ) );

  /* Build XML/Edit DOM. */
  connect( ui->deleteElementButton,         SIGNAL( clicked() ),       this, SLOT( deleteElementFromDOM() ) );
  connect( ui->addElementButton,            SIGNAL( clicked() ),       this, SLOT( addChildElementToDOM() ) );
  connect( ui->dockWidgetSaveButton,        SIGNAL( clicked() ),       this, SLOT( saveDirectEdit() ) );

  /* Various other actions. */
  connect( ui->actionSuperUserMode,         SIGNAL( toggled( bool ) ), this, SLOT( switchSuperUserMode( bool ) ) );
  connect( ui->expandAllCheckBox,           SIGNAL( clicked( bool ) ), this, SLOT( collapseOrExpandTreeWidget( bool ) ) );
  connect( ui->actionExit,                  SIGNAL( triggered() ),     this, SLOT( close() ) );

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

  m_domDoc = new QDomDocument;

  /* If the interface was successfully initialised, prompt the user to choose a database
    connection for this session. */
  showKnownDBForm( GCKnownDBForm::ShowAll );

  /* Everything happens automagically. */
  XmlSyntaxHighlighter *highLighter = new XmlSyntaxHighlighter( ui->dockWidgetTextEdit );
  Q_UNUSED( highLighter );
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
    QString errMsg( "No active database set, please set one for this session." );
    showErrorMessageBox( errMsg );
    showKnownDBForm( GCKnownDBForm::ShowAll );
    return;
  }

  QString fileName = QFileDialog::getOpenFileName( this, "Open File", QDir::homePath(), "XML Files (*.*)" );

  /* If the user clicked "OK". */
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

    resetDOM();

    QTextStream inStream( &file );
    QString fileContent( inStream.readAll() );
    file.close();

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
      we need to warn him/her of this fact and let them either switch to the DB that they need, or
      create a new DB connection for the new XML profile. */
    if( !m_dbInterface->knownRootElements().contains( m_domDoc->documentElement().tagName() ) &&
        !m_superUserMode )
    {
      do
      {
        QMessageBox::warning( this,
                              "Unknown XML Style",
                              "The current active database has no knowledge of the\n"
                              "specific XML style (the elements, attributes, attribute values and\n"
                              "all the associations between them) of the document you are trying to open.\n\n"
                              "You can either:\n\n"
                              "1. Select an existing database that describes this type of XML, or\n"
                              "2. Switch to \"Super User\" mode and open the file again to import it to the database." );

        showKnownDBForm( GCKnownDBForm::SelectAndExisting );

      } while( !m_dbInterface->knownRootElements().contains( m_domDoc->documentElement().tagName() ) &&
               !m_userCancelled );

      /* If the user selected a database that fits, continue. */
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
    else if( m_superUserMode )
    {
      /* If the user is a super user, he/she might want to import the XML profile to the
        current database. */
      QMessageBox::StandardButton button = QMessageBox::question( this,
                                                                  "Import XML?",
                                                                  "Would you like to import the XML document to the active database?",
                                                                  QMessageBox::Yes | QMessageBox::No,
                                                                  QMessageBox::No );

      if( button == QMessageBox::Yes )
      {
        processDOMDoc();

        /* Update the DB in one go. */
        if( !m_dbInterface->batchProcessDOMDocument( m_domDoc ) )
        {
          showErrorMessageBox( m_dbInterface->getLastError() );
        }
      }
      else
      {
        resetDOM();
      }
    }
    else
    {
      /* Finally, if the user selected a database that knows of this particular XML profile,
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
  m_domDoc->clear();
  ui->dockWidgetTextEdit->clear();
  m_treeItemNodes.clear();
  ui->treeWidget->clear();
  resetTableWidget();

  m_currentXMLFileName = "";

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
  ui->dockWidgetTextEdit->setPlainText( m_domDoc->toString( 2 ) );

  /* If the user just added the root element, we need to make sure that they don't
    try to add it again...it happens. */
  if( !m_rootElementSet )
  {
    ui->treeWidget->setCurrentItem( item, 0 );
    treeWidgetItemActivated( item, 0 );
    m_rootElementSet = true;
  }
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

        ui->dockWidgetTextEdit->setPlainText( m_domDoc->toString( 2 ) );
        ui->dockWidgetTextEdit->find( scrollAnchorText( m_treeItemNodes.value( item ).toElement() ) );
        ui->dockWidgetTextEdit->ensureCursorVisible();
      }
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::treeWidgetItemActivated( QTreeWidgetItem *item, int column )
{
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
  QString elementName = item->text( column );
  QStringList attributes = m_dbInterface->attributes( elementName, success );

  /* This is more for debugging than for end-user functionality. */
  if( !success )
  {
    showErrorMessageBox( m_dbInterface->getLastError() );
  }

  for( int i = 0; i < attributes.count(); ++i )
  {
    QTableWidgetItem *label = new QTableWidgetItem( attributes.at( i ) );

    /* Items are editable by default, disable this option if not in Super User mode. */
    if( !m_superUserMode )
    {
      label->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable );
    }

    ui->tableWidget->setRowCount( i + 1 );
    ui->tableWidget->setItem( i, 0, label );

    QComboBox *attributeCombo = new QComboBox;
    attributeCombo->addItems( m_dbInterface->attributeValues( elementName, attributes.at( i ), success ) );    

    /* This is more for debugging than for end-user functionality. */
    if( !success )
    {
      showErrorMessageBox( m_dbInterface->getLastError() );
    }

    QDomElement element = m_treeItemNodes.value( item );
    QString attributeValue = element.attribute( attributes.at( i ) );

    /* If we are still in the process of building the document, the attribute value will
      be empty since it has never been set before.  For this particular case,
      calling "findText" will result in a null pointer exception. */
    if( !attributeValue.isEmpty() )
    {
      attributeCombo->setCurrentIndex( attributeCombo->findText( attributeValue ) );
    }

    /* Attempting the connection before we've set the current index causes the
      "attributeValueChanged" slot to be called too early, resulting in a segmentation
      fault due to value conflicts/missing values. */
    connect( attributeCombo, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( attributeValueChanged( QString ) ) );

    attributeCombo->setEditable( true );
    ui->tableWidget->setCellWidget( i, 1, attributeCombo );
    m_comboBoxes.insert( attributeCombo, i );

    /* This will point the current combo box member to the combo that's been activated
      in the table widget (used in "attributeValueChanged" to obtain the row number the
      combo box appears in in the table widget, etc, etc). */
    connect( attributeCombo, SIGNAL(activated( int ) ), m_signalMapper, SLOT( map() ) );
    m_signalMapper->setMapping( attributeCombo, attributeCombo );
  }

  /* Populate the "add element" combo box with the known first level children of the
    highlighted element. */
  ui->addElementComboBox->clear();
  ui->addElementComboBox->addItems( m_dbInterface->children( elementName, success ) );
  toggleAddElementWidgets();

  /* This is more for debugging than for end-user functionality. */
  if( !success )
  {
    showErrorMessageBox( m_dbInterface->getLastError() );
  }

  /* Unset flag. */
  m_wasTreeItemActivated = false;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setCurrentComboBox( QComboBox *combo )
{
  m_currentCombo = combo;
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

void GCMainWindow::setActiveAttributeName( QTableWidgetItem *item )
{
  m_activeAttributeName = item->text();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::attributeNameChanged( QTableWidgetItem *item )
{
  /* Don't execute the logic if a tree widget item's activating is triggering
    a re-population of the table widget, resulting in this slot being called. */
  if( !m_wasTreeItemActivated )
  {
    /* All attribute name changes will be assumed to be additions, removing an attribute
      with a specific name has to be done explicitly. This slot will only ever be called
      in Super User mode. */
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
      QComboBox *attributeValueCombo = dynamic_cast< QComboBox* >( ui->tableWidget->cellWidget( ui->tableWidget->currentRow(), 1 ) );

      if( attributeValueCombo )
      {
        currentElement.removeAttribute( m_activeAttributeName );
        currentElement.setAttribute( item->text(), attributeValueCombo->currentText() );

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

      ui->dockWidgetTextEdit->setPlainText( m_domDoc->toString( 2 ) );
      ui->dockWidgetTextEdit->find( scrollAnchorText( currentElement ) );
      ui->dockWidgetTextEdit->ensureCursorVisible();
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::attributeValueChanged( const QString &value )
{
  QTreeWidgetItem *currentItem = ui->treeWidget->currentItem();
  QDomElement currentElement = m_treeItemNodes.value( currentItem );

  /* The current attribute will be displayed in the first column (next to the
    combo box which will be the actual current item). */
  QString currentAttributeName = ui->tableWidget->item( m_comboBoxes.value( m_currentCombo ), 0 )->text();
  currentElement.setAttribute( currentAttributeName, value );

  ui->dockWidgetTextEdit->setPlainText( m_domDoc->toString( 2 ) );
  ui->dockWidgetTextEdit->find( scrollAnchorText( currentElement ) );
  ui->dockWidgetTextEdit->ensureCursorVisible();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::attributeValueAdded( const QString &value )
{
  /* It would have been nice if we could simply call "attributeValueChanged"
    here, but we need to know which element and attribute the new value is
    asociated with in any case so we may as well just do it all here. */
  QTreeWidgetItem *currentItem = ui->treeWidget->currentItem();
  QDomElement currentElement = m_treeItemNodes.value( currentItem );

  /* The current attribute will be displayed in the first column (next to the
    combo box which will be the actual current item). */
  QString currentAttributeName = ui->tableWidget->item( m_comboBoxes.value( m_currentCombo ), 0 )->text();
  currentElement.setAttribute( currentAttributeName, value );

  ui->dockWidgetTextEdit->setPlainText( m_domDoc->toString( 2 ) );
  ui->dockWidgetTextEdit->find( scrollAnchorText( currentElement ) );
  ui->dockWidgetTextEdit->ensureCursorVisible();

  if( !m_dbInterface->updateAttributeValues( currentElement.tagName(),
                                             currentAttributeName,
                                             QStringList( value ) ) )
  {
    showErrorMessageBox( m_dbInterface->getLastError() );
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

  ui->dockWidgetTextEdit->setPlainText( m_domDoc->toString( 2 ) );
  ui->dockWidgetTextEdit->find( scrollAnchorText( parentNode.toElement() ) );
  ui->dockWidgetTextEdit->ensureCursorVisible();
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

    ui->dockWidgetTextEdit->setPlainText( m_domDoc->toString( 2 ) );
    ui->dockWidgetTextEdit->find( scrollAnchorText( newElement ) );
    ui->dockWidgetTextEdit->ensureCursorVisible();

    /* If the user just added the root element, we need to make sure that they don't
    try to add it again...it happens. */
    if( !m_rootElementSet )
    {
      ui->treeWidget->setCurrentItem( newItem, 0 );
      treeWidgetItemActivated( newItem, 0 );
      m_rootElementSet = true;
    }
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

void GCMainWindow::addNewDB()
{
  QString file = QFileDialog::getSaveFileName( this, "Add New Database", QDir::homePath(), "Database Files (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    addDBConnection( file );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addExistingDB()
{
  QString file = QFileDialog::getOpenFileName( this, "Add Existing Database", QDir::homePath(), "Database Files (*.db)" );

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

  QMessageBox::StandardButton button = QMessageBox::question( this,
                                                              "Set Session",
                                                              "Would you like to set the new database as active?",
                                                              QMessageBox::Yes | QMessageBox::No,
                                                              QMessageBox::Yes );

  if( button == QMessageBox::Yes )
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
    /* If the user set an empty database, prompt to populate it. */
    if( m_dbInterface->knownElements().size() < 1 )
    {
      QMessageBox::warning( this,
                            "Empty Database",
                            "The current active database is completely empty (aka \"entirely useless\").\n"
                            "You can either:\n"
                            "1. Select a different (populated) database and continue working, or\n"
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
    QString error = QString( "Failed to remove database \"%1\": [%2]" ).arg( dbName )
                    .arg( m_dbInterface->getLastError() );
    showErrorMessageBox( error );
  }

  /* If the user removed the active DB for this session, we need to know
    what he/she intends to replace it with. */
  if( !m_dbInterface->hasActiveSession() )
  {
    QString errMsg( "The active database has been removed, please set another as active." );
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
    QMessageBox::StandardButton button = QMessageBox::warning( this,
                                                               "Warning",
                                                               "Switching database sessions while building an XML document\n"
                                                               "will cause the document to be reset and your work will be lost.  If this is fine, proceed with \"OK\".\n\n"
                                                               "On the other hand, if you wish to keep your work, please hit \"Cancel\" and \n"
                                                               "save the document first before coming back here.",
                                                               QMessageBox::Ok | QMessageBox::Cancel );

    if( button == QMessageBox::Ok )
    {
      resetDOM();
    }
    else
    {
      return;
    }
  }

  showKnownDBForm( GCKnownDBForm::ShowAll );
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

}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::saveDirectEdit()
{

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

  if( super )
  {
    QMessageBox::warning( this,
                          "Super User Mode!",
                          "Absolutely everything you do in this mode is persisted to the\n"
                          "active database and cannot be undone.\n\n"
                          "In other words, if anything goes wrong, it's all your fault..." );
  }

  if( !m_dbInterface->hasActiveSession() )
  {
    showKnownDBForm( GCKnownDBForm::SelectAndExisting );
  }

  m_superUserMode = super;

  ui->addAttributeButton->setVisible( super );
  ui->addAttributeLabel->setVisible( super );
  ui->addAttributeLineEdit->setVisible( super );

  /* The user must see these actions exist, but shouldn't be able to access
    them except when in super user mode. */
  ui->actionAddNewDatabase->setEnabled( super );
  ui->actionRemoveDatabase->setEnabled( super );

  /* Needed to reset all the tree widget item's "editable" flags
    to whatever the current mode allows. */
  QList< QTreeWidgetItem* > itemList = m_treeItemNodes.keys();

  if( super )
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
}

/*--------------------------------------------------------------------------------------*/

QString GCMainWindow::scrollAnchorText( const QDomElement &element )
{
  QString anchor( "<" );
  anchor += element.tagName();

  bool success( false );
  QStringList attributes = m_dbInterface->attributes( element.tagName(), success );

  if( success && !attributes.empty() )
  {
    for( int i = 0; i < attributes.size(); ++i )
    {
      anchor += " ";

      QString attribute = attributes.at( i );
      anchor += attribute;
      anchor += "=\"";

      QString attributeValue = element.attribute( attribute );
      anchor += attributeValue;
      anchor += "\"";
    }

    anchor += ( "/>" );
  }
  else if( attributes.empty() )
  {
    anchor += ( ">" );
  }
  else
  {
    showErrorMessageBox( m_dbInterface->getLastError() );
  }

  return anchor;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showErrorMessageBox( const QString &errorMsg )
{
  QMessageBox::critical( this, "Error!", errorMsg );
}

/*--------------------------------------------------------------------------------------*/
