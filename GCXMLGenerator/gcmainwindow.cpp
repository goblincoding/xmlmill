#include "gcmainwindow.h"
#include "ui_gcmainwindow.h"
#include "db/gcdatabaseinterface.h"
#include "db/gcknowndbform.h"
#include "utils/gcmessagebox.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QComboBox>

/*--------------------------------------------------------------------------------------*/

GCMainWindow::GCMainWindow( QWidget *parent ) :
  QMainWindow         ( parent ),
  ui                  ( new Ui::GCMainWindow ),
  m_dbInterface       ( new GCDataBaseInterface ),
  m_domDoc            (),
  m_currentXMLFileName( "" ),
  m_userCancelled     ( false ),
  m_superUserMode     ( false ),
  m_treeItemNodes     ()
{
  ui->setupUi( this );

  /* Disable super user options. */
  ui->addAttributeButton->setVisible( false );
  ui->addAttributeLabel->setVisible( false );
  ui->addAttributeLineEdit->setVisible( false );

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
  connect( ui->actionSuperUserMode,         SIGNAL( toggled( bool ) ), this, SLOT( toggleSuperUserMode( bool ) ) );
  connect( ui->expandAllCheckBox,           SIGNAL( clicked( bool ) ), this, SLOT( collapseOrExpandTreeWidget( bool ) ) );
  connect( ui->actionExit,                  SIGNAL( triggered() ),     this, SLOT( close() ) );

  /* Everything tree widget related. */
  connect( ui->treeWidget,                  SIGNAL( itemChanged  ( QTreeWidgetItem*, int ) ), this, SLOT  ( treeWidgetItemChanged  ( QTreeWidgetItem*, int ) ) );
  connect( ui->treeWidget,                  SIGNAL( itemClicked  ( QTreeWidgetItem*, int ) ), this, SLOT  ( treeWidgetItemActivated( QTreeWidgetItem*, int ) ) );
  connect( ui->treeWidget,                  SIGNAL( itemActivated( QTreeWidgetItem*, int ) ), this, SLOT  ( treeWidgetItemActivated( QTreeWidgetItem*, int ) ) );

  /* Initialise the database interface and retrieve the list of database names (this will
    include the path references to the ".db" files). */
  if( !m_dbInterface->initialise() )
  {
    showErrorMessageBox( m_dbInterface->getLastError() );
    this->close();
  }

  /* If the interface was successfully initialised, prompt the user to choose a database
    connection for this session. */
  showKnownDBForm();
}

/*--------------------------------------------------------------------------------------*/

GCMainWindow::~GCMainWindow()
{
  // TODO: Give the user the option to save the current XML file.

  delete m_dbInterface;
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::openXMLFile()
{
  if( m_dbInterface->hasActiveSession() )
  {
    QString fileName = QFileDialog::getOpenFileName( this, "Open File", QDir::homePath(), "XML Files (*.*)" );

    /* If the user clicked "OK". */
    if( !fileName.isEmpty() )
    {
      m_currentXMLFileName = fileName;
      QFile file( m_currentXMLFileName );

      if( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
      {
        resetDOM();

        QTextStream inStream( &file );
        QString fileContent( inStream.readAll() );

        QString xmlErr( "" );
        int     line  ( -1 );
        int     col   ( -1 );

        if( m_domDoc.setContent( fileContent, &xmlErr, &line, &col ) )
        {
          /* If the user is opening an XML file of a kind that isn't supported by the current active session,
            we need to warn the user of this fact and let them either switch to the DB that they need, or
            create a new DB connection for the new XML file type. */
          if( !m_dbInterface->knownRootElements().contains( m_domDoc.documentElement().tagName() ) &&
              !m_superUserMode )
          {
            do
            {
              GCMessageBox messageBox( GCMessageBox::UnknownXMLStyle );
              messageBox.exec();
              showKnownDBForm();
            } while( !m_dbInterface->knownRootElements().contains( m_domDoc.documentElement().tagName() ) &&
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
            processDOMDoc();
          }
        }
        else
        {
          QString errorMsg = QString( "XML is broken - Error [%1], line [%2], column [%3])." ).arg( xmlErr ).arg( line ).arg( col );
          showErrorMessageBox( errorMsg );
          resetDOM();
          m_currentXMLFileName = "";
        }

        file.close();
      }
      else
      {
        QString errorMsg = QString( "Failed to open file \"%1\" - [%2]" ).arg( fileName ).arg( file.errorString() );
        showErrorMessageBox( errorMsg );
        resetDOM();
        m_currentXMLFileName = "";
      }
    }
  }
  else
  {
    QString errMsg( "No active database set, please set one for this session." );
    showErrorMessageBox( errMsg );
    showKnownDBForm();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::newXMLFile()
{
  resetDOM();
  m_currentXMLFileName = "";

  ui->addElementComboBox->clear();
  ui->addElementComboBox->addItems( m_dbInterface->knownRootElements() );
  toggleAddElementWidgets();

  ui->actionSave->setEnabled( true );
  ui->actionSaveAs->setEnabled( true );
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
      QString errMsg = QString( "Failed to save file \"%1\" - [%2]." ).arg( m_currentXMLFileName ).arg( file.errorString() );
      showErrorMessageBox( errMsg );
    }
    else
    {
      QTextStream outStream( &file );
      outStream << m_domDoc.toString( 2 );
      file.close();
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

void GCMainWindow::resetDOM()
{
  m_domDoc.clear();
  ui->dockWidgetTextEdit->clear();
  m_treeItemNodes.clear();
  ui->treeWidget->clear();
  resetTableWidget();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::processDOMDoc()
{
  ui->treeWidget->clear();            // also deletes current items
  m_treeItemNodes.clear();
  resetTableWidget();

  QDomElement root = m_domDoc.documentElement();

  /* We want to show the document's root element in the tree widget as well. */
  QTreeWidgetItem *item = new QTreeWidgetItem;
  item->setText( 0, root.tagName() );

  if( m_superUserMode )
  {
    item->setFlags( item->flags() | Qt::ItemIsEditable );
  }

  ui->treeWidget->invisibleRootItem()->addChild( item );  // takes ownership
  m_treeItemNodes.insert( item, root );

  /* Now we can recursively stick the rest of the elements into our widget. */
  populateTreeWidget( root, item );

  /* Enable file save options. */
  ui->actionSave->setEnabled( true );
  ui->actionSaveAs->setEnabled( true );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::populateTreeWidget( const QDomElement &parentElement, QTreeWidgetItem *parentItem )
{
  QDomElement element = parentElement.firstChildElement();

  while ( !element.isNull() )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText( 0, element.tagName() );

    if( m_superUserMode )
    {
      item->setFlags( item->flags() | Qt::ItemIsEditable );
    }

    parentItem->addChild( item );             // takes ownership
    m_treeItemNodes.insert( item, element );

    populateTreeWidget( element, item );
    element = element.nextSiblingElement();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::resetTableWidget()
{
  ui->tableWidget->clearContents();   // also deletes current items
  ui->tableWidget->setRowCount( 0 );
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::updateDataBase()
{  
  if( m_dbInterface->hasActiveSession() )
  {
    // Get hold of XML node values here...
  }
  else
  {
    QString errMsg( "No active database set, please set one for this session." );
    showErrorMessageBox( errMsg );
    showKnownDBForm();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::treeWidgetItemChanged( QTreeWidgetItem *item, int column )
{
  /* This slot will only be called in super user mode so we can safely keep the functionality
    as it is (i.e. updating the database alongside the DOM) without any other explicit checks. */
  if( m_treeItemNodes.contains( item ) )
  {
    QString elementName      = item->text( column );
    QString previousName  = m_treeItemNodes.value( item ).toElement().tagName();

    /* Watch out for empty strings. */
    if( elementName.isEmpty() )
    {
      showErrorMessageBox( "Sorry, but we don't know what to do with empty names..." );
      item->setText( column, previousName );
    }
    else
    {
      if( elementName != previousName )
      {
        /* Update the element names in our active DOM doc (since m_treeItemNodes
        contains shallow copied QDomElements, the change will automatically
        be available to the map as well) and the tree widget. */
        QDomNodeList list = m_domDoc.elementsByTagName( previousName );

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
                                   m_dbInterface->children  ( previousName, success ),
                                   attributes );

        foreach( QString attribute, attributes )
        {
          m_dbInterface->updateAttributeValues( elementName, attribute, m_dbInterface->attributeValues( previousName, attribute, success ) );
        }

        if( !success )
        {
          showErrorMessageBox( m_dbInterface->getLastError() );
        }

        ui->dockWidgetTextEdit->setPlainText( m_domDoc.toString() );
      }
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::treeWidgetItemActivated( QTreeWidgetItem *item, int column )
{
  resetTableWidget();

  /* Get only the attributes currently assigned to the element
    corresponding to this item (and the lists of associated
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

    if( m_superUserMode )
    {
      label->setFlags( label->flags() | Qt::ItemIsEditable );
    }

    ui->tableWidget->setRowCount( i + 1 );
    ui->tableWidget->setItem( i, 0, label );

    QComboBox *attributeCombo = new QComboBox;
    attributeCombo->addItems( m_dbInterface->attributeValues( elementName, attributes.at( i ), success ) );

    /* Get the current value assigned to the element associated with this tree widget item. */
    QDomElement element = m_treeItemNodes.value( item );
    QString attributeValue = element.attribute( attributes.at( i ) );
    attributeCombo->setCurrentIndex( attributeCombo->findText( attributeValue ) );

    /* This is more for debugging than for end-user functionality. */
    if( !success )
    {
      showErrorMessageBox( m_dbInterface->getLastError() );
    }

    attributeCombo->setEditable( true );
    ui->tableWidget->setCellWidget( i, 1, attributeCombo );
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
                                                              "Would you like to set the new connection as active?",
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
      showKnownDBForm();
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::setSessionDB( QString dbName )
{
  if( !m_dbInterface->setSessionDB( dbName ) )
  {
    QString error = QString( "Failed to set session \"%1\" as active - [%2]" ).arg( dbName )
                    .arg( m_dbInterface->getLastError() );
    showErrorMessageBox( error );
  }
  else
  {
    /* If we have an empty DOM doc, load the list of known document root elements
      to start the document building process. */
    if( m_domDoc.documentElement().isNull() )
    {
      ui->addElementComboBox->clear();
      ui->addElementComboBox->addItems( m_dbInterface->knownRootElements() );
      toggleAddElementWidgets();
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::removeDB()
{
  showKnownDBForm( true );
}

/*--------------------------------------------------------------------------------------*/


void GCMainWindow::removeDBConnection( QString dbName )
{
  if( !m_dbInterface->removeDatabase( dbName ) )
  {
    QString error = QString( "Failed to remove database \"%1\" - [%2]" ).arg( dbName )
                    .arg( m_dbInterface->getLastError() );
    showErrorMessageBox( error );
  }

  /* If the user removed the active DB for this session, we need to know
    what he/she intends to replace it with. */
  if( !m_dbInterface->hasActiveSession() )
  {
    showKnownDBForm();
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

  showKnownDBForm();
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
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addChildElementToDOM()
{
  QString newElementName = ui->addElementComboBox->currentText();

  /* Update the tree widget. */
  QTreeWidgetItem *newItem = new QTreeWidgetItem;
  newItem->setText( 0, newElementName );

  if( m_superUserMode )
  {
    newItem->setFlags( newItem->flags() | Qt::ItemIsEditable );
  }

  /* Update the current DOM document. */
  QDomElement newElement = m_domDoc.createElement( newElementName );

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
      a new file to be created, do it automatically (we can't call newXMLFile here since
      it resets the DOM document). */
    m_currentXMLFileName = "";
    ui->actionSave->setEnabled( true );
    ui->actionSaveAs->setEnabled( true );

    ui->treeWidget->invisibleRootItem()->addChild( newItem );  // takes ownership
    m_domDoc.appendChild( newElement );
  }

  /* Keep everything in sync in the map. */
  m_treeItemNodes.insert( newItem, newElement );

  ui->dockWidgetTextEdit->setPlainText( m_domDoc.toString() );
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

void GCMainWindow::showKnownDBForm( bool remove )
{
  GCKnownDBForm *knownDBForm = new GCKnownDBForm( m_dbInterface->getDBList(), remove, this );

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

void GCMainWindow::toggleSuperUserMode( bool super )
{

  // TODO: Display warning regarding all edits being persisted to DB
  m_superUserMode = super;

  ui->addAttributeButton->setVisible( super );
  ui->addAttributeLabel->setVisible( super );
  ui->addAttributeLineEdit->setVisible( super );

  /* Needed to reset reset all the tree widget item's "editable" flags
    to whatever the current mode allows. */
  processDOMDoc();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showErrorMessageBox( const QString &errorMsg )
{
  QMessageBox::critical( this, "Error!", errorMsg );
}

/*--------------------------------------------------------------------------------------*/
