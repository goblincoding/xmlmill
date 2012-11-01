#include "gcmainwindow.h"
#include "ui_gcmainwindow.h"
#include "db/gcdatabaseinterface.h"
#include "db/gcsessiondbform.h"
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
  m_treeItemNodes     ()
{
  ui->setupUi( this );

  connect( ui->treeWidget, SIGNAL( itemChanged( QTreeWidgetItem*,int ) ),
           this,           SLOT  ( treeWidgetItemChanged( QTreeWidgetItem*,int ) ) );

  connect( ui->treeWidget, SIGNAL( itemClicked( QTreeWidgetItem*,int ) ),
           this,           SLOT  ( treeWidgetItemClicked( QTreeWidgetItem*,int ) ) );

  /* XML File related. */
  connect( ui->actionOpen,   SIGNAL( triggered() ), this, SLOT( openXMLFile() ) );
  connect( ui->actionSave,   SIGNAL( triggered() ), this, SLOT( saveXMLFile() ) );
  connect( ui->actionSaveAs, SIGNAL( triggered() ), this, SLOT( saveXMLFileAs() ) );

  /* Database related. */
  connect( ui->actionAddNewDatabase,        SIGNAL( triggered() ), this, SLOT( addNewDB() ) );
  connect( ui->actionAddExistingDatabase,   SIGNAL( triggered() ), this, SLOT( addExistingDB() ) );
  connect( ui->actionRemoveDatabase,        SIGNAL( triggered() ), this, SLOT( removeDB() ) );
  connect( ui->actionSwitchSessionDatabase, SIGNAL( triggered() ), this, SLOT( switchDBSession() ) );

  /* Build XML. */
  connect( ui->buildXMLUpdateButton, SIGNAL( clicked() ), this, SLOT( updateDataBase() ) );
  connect( ui->buildXMLDeleteButton, SIGNAL( clicked() ), this, SLOT( deleteElement() ) );
  connect( ui->addAsChildButton,     SIGNAL( clicked() ), this, SLOT( addAsChild() ) );
  connect( ui->addAsSiblingButton,   SIGNAL( clicked() ), this, SLOT( addAsSibling() ) );

  /* Edit XML store. */
  connect( ui->editXMLAddButton,              SIGNAL( clicked() ), this, SLOT( updateDataBase() ) );
  connect( ui->editXMLDeleteAttributesButton, SIGNAL( clicked() ), this, SLOT( deleteAttributeValuesFromDB() ) );
  connect( ui->editXMLDeleteElementsButton,   SIGNAL( clicked() ), this, SLOT( deleteElementFromDB() ) );

  /* Direct DOM edit. */
  connect( ui->dockWidgetRevertButton, SIGNAL( clicked() ), this, SLOT( revertDirectEdit() ) );
  connect( ui->dockWidgetSaveButton,   SIGNAL( clicked() ), this, SLOT( saveDirectEdit() ) );

  /* Initialise the database interface and retrieve the list of database names (this will
    include the path references to the ".db" files). */
  if( !m_dbInterface->initialise() )
  {
    QMessageBox::critical( this, "Error!", m_dbInterface->getLastError() );
    this->close();
  }

  /* If the interface was successfully initialised, prompt the user to choose a database
    connection for this session. */
  showSessionForm();  
}

/*--------------------------------------------------------------------------------------*/

GCMainWindow::~GCMainWindow()
{
  delete m_dbInterface;
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::openXMLFile()
{
  if( m_dbInterface->hasActiveSession() )
  {
    /* Loading an entire XML file is a big operation (with regards to potential changes
    introduced) so let's query the user if he/she would like to add the XML content
    to the current active database.  For other operations (such as element and attribute
    name changes) the user needs to explicitly update the database. */
    QMessageBox::Button button = QMessageBox::question( this,
                                                        "Update database?",
                                                        "All the elements, attributes and attribute\n"
                                                        "values in this XML document will be saved to\n"
                                                        "the current active database session.\n \n"
                                                        "Is that OK?",
                                                        QMessageBox::Yes | QMessageBox::No,
                                                        QMessageBox::Yes );
    if( button == QMessageBox::Yes )
    {
      QString fileName = QFileDialog::getOpenFileName( this, "Open File", QDir::homePath(), "XML Files (*.*)" );

      /* If the user clicked "OK". */
      if( !fileName.isEmpty() )
      {
        m_currentXMLFileName = fileName;
        QFile file( m_currentXMLFileName );

        if( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
          QTextStream inStream( &file );
          QString xmlErr( "" );
          int     line  ( -1 );
          int     col   ( -1 );

          if( m_domDoc.setContent( inStream.readAll(), &xmlErr, &line, &col ) )
          {
            processDOMDoc();
          }
          else
          {
            QString errorMsg = QString( "XML is broken - Error [%1], line [%2], column [%3])." ).arg( xmlErr ).arg( line ).arg( col );
            showErrorMessageBox( errorMsg );
          }

          file.close();
        }
        else
        {
          QString errorMsg = QString( "Failed to open file \"%1\" - [%2]" ).arg( fileName ).arg( file.errorString() );
          showErrorMessageBox( errorMsg );
        }
      }
    }
  }
  else
  {
    QString errMsg( "No current DB active, please set one for this session." );
    showErrorMessageBox( errMsg );
    showSessionForm();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::saveXMLFile()
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

void GCMainWindow::processDOMDoc()
{
  ui->treeWidget->clear();    // also deletes current items
  m_treeItemNodes.clear();

  QDomElement root = m_domDoc.documentElement();

  /* We want to show the document's root element in the tree widget as well. */
  QTreeWidgetItem *item = new QTreeWidgetItem;
  item->setText( 0, root.tagName() );
  item->setFlags( item->flags() | Qt::ItemIsEditable );
  ui->treeWidget->invisibleRootItem()->addChild( item );  // takes ownership

  m_treeItemNodes.insert( item, root );

  /* Now we can recursively stick the rest of the elements into our widget. */
  populateTreeWidget( root, item );

  /* Update the DB in one go. */
  if( !m_dbInterface->batchProcessDOMDocument( m_domDoc ) )
  {
    showErrorMessageBox( m_dbInterface->getLastError() );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::populateTreeWidget( const QDomElement &parentElement, QTreeWidgetItem *parentItem )
{
  QDomElement element = parentElement.firstChildElement();

  while ( !element.isNull() )
  {
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText( 0, element.tagName() );
    item->setFlags( item->flags() | Qt::ItemIsEditable );
    parentItem->addChild( item );   // takes ownership

    m_treeItemNodes.insert( item, element );

    populateTreeWidget( element, item );
    element = element.nextSiblingElement();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::populateDBTables( const QDomElement &element )
{  

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
    QString errMsg( "No current DB active, please set one for this session." );
    showErrorMessageBox( errMsg );
    showSessionForm();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::treeWidgetItemChanged( QTreeWidgetItem *item, int column )
{
  if( m_treeItemNodes.contains( item ) )
  {
    QString itemName      = item->text( column );
    QString previousName  = m_treeItemNodes.value( item ).toElement().tagName();

    if( itemName != previousName )
    {
      /* Update the element names in our active DOM doc (since m_treeItemNodes
        contains shallow copied QDomElements, the change will automatically
        be available to the map as well). */
      QDomNodeList list = m_domDoc.elementsByTagName( previousName );

      for( int i = 0; i < list.count(); ++i )
      {
        list.at( i ).toElement().setTagName( itemName );
      }

      /* Re-populate the tree widget (and update the DB) to reflect the changes
        (note that the "old" elements won't be removed from the DB, the new ones
         will simply be added.  All removals must be executed explicitly). */
      //processDOMDoc();

      //TODO - Update single records only!  Not entire DOM!

    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::treeWidgetItemClicked( QTreeWidgetItem *item, int column )
{
  ui->tableWidget->clear();    // also deletes current items

  /* Get only the attributes currently assigned to the element
    corresponding to this item (and the lists of associated
    values for these attributes) and populate our table widget. */
  bool success( false );
  QString itemName = item->text( column );
  QStringList attributes = m_dbInterface->attributes( itemName, success );

  /* This is more for debugging than for end-user functionality. */
  if( !success )
  {
    showErrorMessageBox( m_dbInterface->getLastError() );
  }

  for( int i = 0; i < attributes.count(); ++i )
  {
    QTableWidgetItem *label = new QTableWidgetItem( attributes.at( i ) );
    label->setFlags( label->flags() | Qt::ItemIsEditable );
    ui->tableWidget->setRowCount( i + 1 );
    ui->tableWidget->setItem( i, 0, label );

    QComboBox *attributeCombo = new QComboBox;
    attributeCombo->addItems( m_dbInterface->attributeValues( itemName, attributes.at( i ), success ) );

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
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addNewDB()
{
  QString file = QFileDialog::getSaveFileName( this, "Add New Database", QDir::homePath(), "DB Files (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    addDBConnection( file );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addExistingDB()
{
  QString file = QFileDialog::getOpenFileName( this, "Add Existing Database", QDir::homePath(), "DB Files (*.db)" );

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
    QString error = QString( "Failed to add connection - [%1]" ).arg( m_dbInterface->getLastError() );
    showErrorMessageBox( error );
  }

  QMessageBox::StandardButton button = QMessageBox::question( this,
                                                              "Set session",
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
      showSessionForm();
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
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::removeDB()
{
  showSessionForm( true );
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
    showSessionForm();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::switchDBSession()
{
  showSessionForm();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::deleteElement()
{

}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addAsChild()
{

}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::addAsSibling()
{

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

void GCMainWindow::showSessionForm( bool remove )
{
  GCSessionDBForm *sessionForm = new GCSessionDBForm( m_dbInterface->getDBList(), remove, this );

  /* If we don't have an active DB session, it's probably at program
    start-up and the user wishes to exit the application by clicking "Cancel". */
  if( !m_dbInterface->hasActiveSession() )
  {
    connect( sessionForm, SIGNAL( userCancelled() ),       this, SLOT( close() ) );
  }

  connect( sessionForm,   SIGNAL( newConnection() ),       this, SLOT( addNewDB() ) );
  connect( sessionForm,   SIGNAL( existingConnection() ),  this, SLOT( addExistingDB() ) );
  connect( sessionForm,   SIGNAL( dbSelected( QString ) ), this, SLOT( setSessionDB( QString ) ) );
  connect( sessionForm,   SIGNAL( dbRemoved ( QString ) ), this, SLOT( removeDBConnection( QString ) ) );

  sessionForm->show();
}

/*--------------------------------------------------------------------------------------*/

void GCMainWindow::showErrorMessageBox( const QString &errorMsg )
{
  QMessageBox::information( this, "Error!", errorMsg );
}

/*--------------------------------------------------------------------------------------*/
