#include "gcmainwindow.h"
#include "ui_gcmainwindow.h"
#include "db/gcdatabaseinterface.h"
#include "db/gcsessiondbform.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>

/*-------------------------------------------------------------*/

GCMainWindow::GCMainWindow( QWidget *parent ) :
  QMainWindow        ( parent ),
  ui                 ( new Ui::GCMainWindow ),
  m_dbInterface      ( new GCDataBaseInterface ),
  m_elements         (),
  m_attributes       (),
  m_domDoc           (),
  m_fileName         ( "" )
{
  ui->setupUi( this );

  /* XML File related. */
  connect( ui->actionOpen,   SIGNAL( triggered() ), this, SLOT( openFile() ) );
  connect( ui->actionSave,   SIGNAL( triggered() ), this, SLOT( saveFile() ) );
  connect( ui->actionSaveAs, SIGNAL( triggered() ), this, SLOT( saveFileAs() ) );

  /* Database related. */
  connect( ui->actionAddNewDatabase,        SIGNAL( triggered() ), this, SLOT( addNewDB() ) );
  connect( ui->actionAddExistingDatabase,   SIGNAL( triggered() ), this, SLOT( addExistingDB() ) );
  connect( ui->actionRemoveDatabase,        SIGNAL( triggered() ), this, SLOT( removeDB() ) );
  connect( ui->actionSwitchSessionDatabase, SIGNAL( triggered() ), this, SLOT( switchDBSession() ) );

  /* Build XML. */
  connect( ui->buildXMLAddButton,    SIGNAL( clicked() ), this, SLOT( addNewElement() ) );
  connect( ui->buildXMLDeleteButton, SIGNAL( clicked() ), this, SLOT( deleteElement() ) );
  connect( ui->addAsChildButton,     SIGNAL( clicked() ), this, SLOT( addAsChild() ) );
  connect( ui->addAsSiblingButton,   SIGNAL( clicked() ), this, SLOT( addAsSibling() ) );

  /* Edit XML store. */
  connect( ui->editXMLAddButton,              SIGNAL( clicked() ), this, SLOT( update() ) );
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

/*-------------------------------------------------------------*/

GCMainWindow::~GCMainWindow()
{
  delete m_dbInterface;
  delete ui;
}

/*-------------------------------------------------------------*/

void GCMainWindow::processInputXML()
{
  ui->treeWidget->clear();
  QDomElement root = m_domDoc.documentElement();
  populateTreeWidget( root, ui->treeWidget->invisibleRootItem() );
  updateDataBase();
}

/*-------------------------------------------------------------*/

void GCMainWindow::populateTreeWidget( const QDomElement &parentElement, QTreeWidgetItem *parentItem )
{
  QDomElement element = parentElement.firstChildElement();

  while ( !element.isNull() )
  {
    /* Stick this item into the tree. */
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText( 0, element.tagName() );
    parentItem->addChild( item );

    populateMaps( element );

    populateTreeWidget( element, item );
    element = element.nextSiblingElement();
  }
}

/*-------------------------------------------------------------*/

void GCMainWindow::populateMaps( const QDomElement &element )
{
  /* Collect the element's attributes and update the maps. */
  QDomNamedNodeMap attributes = element.attributes();
  QStringList attrList;

  for( int i = 0; i < attributes.size(); ++i )
  {
    QDomAttr attr = attributes.item( i ).toAttr();

    if( !attr.isNull() )
    {
      attrList.append( attr.name() );

      /* Check if we already know about this attribute, if we do,
        then we add its value to the list of possible values associated
        with this particular attribute, if we don't, then it's a new addition. */
      if( m_attributes.contains( attr.name() ) )
      {
        QStringList tmp( m_attributes.value( attr.name() ) );
        tmp.append( attr.value() );
        tmp.removeDuplicates();
        m_attributes.insert( attr.name(), tmp );
      }
      else
      {
        m_attributes.insert( attr.name(), QStringList( attr.value() ) );
      }
    }
  }

  /* Now that we have all the current element's attributes mapped, let's see
    if we already know about all these attributes or if we have uncovered new ones. */
  if( m_elements.contains( element.tagName() ) )
  {
    QStringList tmp( m_attributes.value( element.tagName() ) );
    tmp.append( attrList );
    tmp.removeDuplicates();
    m_attributes.insert( element.tagName(), tmp );
  }
  else
  {
    m_elements.insert( element.tagName(), attrList );
  }
}

/*-------------------------------------------------------------*/

void GCMainWindow::openFile()
{
  QString fileName = QFileDialog::getOpenFileName( this, "Open File", QDir::homePath(), "XML Files (*.xml)" );

  /* If the user clicked "OK". */
  if( !fileName.isEmpty() )
  {
    m_fileName = fileName;
    QFile file( m_fileName );

    if( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      QTextStream inStream( &file );
      QString xmlErr( "" );
      int     line  ( -1 );
      int     col   ( -1 );

      if( m_domDoc.setContent( inStream.readAll(), &xmlErr, &line, &col ) )
      {
        processInputXML();
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

/*-------------------------------------------------------------*/

void GCMainWindow::saveFile()
{
  QFile file( m_fileName );

  if( !file.open( QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text ) )
  {
    QString errMsg = QString( "Failed to save file \"%1\" - [%2]." ).arg( m_fileName ).arg( file.errorString() );
    showErrorMessageBox( errMsg );
  }
  else
  {
    QTextStream outStream( &file );
    outStream << m_domDoc.toString( 2 );
    file.close();
  }
}

/*-------------------------------------------------------------*/

void GCMainWindow::saveFileAs()
{
  QString file = QFileDialog::getSaveFileName( this, "Save As", QDir::homePath(), "XML Files (*.xml)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    m_fileName = file;
    saveFile();
  }
}

/*-------------------------------------------------------------*/

void GCMainWindow::updateDataBase()
{
  if( m_dbInterface->hasActiveSession() )
  {
    if( !m_dbInterface->addElements( m_elements ) )
    {
      QString errMsg = QString( "Failed to update elements database - [%1]." ).arg( m_dbInterface->getLastError() );
      showErrorMessageBox( errMsg );
    }

    if( !m_dbInterface->addAttributes( m_attributes ) )
    {
      QString errMsg = QString( "Failed to update attributes database - [%1]." ).arg( m_dbInterface->getLastError() );
      showErrorMessageBox( errMsg );
    }
  }
  else
  {
    QString errMsg( "No current DB active, please set one for this session." );
    showErrorMessageBox( errMsg );
    showSessionForm();
  }
}

/*-------------------------------------------------------------*/

void GCMainWindow::addNewDB()
{
  QString file = QFileDialog::getSaveFileName( this, "Add New Database", QDir::homePath(), "DB Files (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    addDBConnection( file );
  }
}

/*-------------------------------------------------------------*/

void GCMainWindow::addExistingDB()
{
  QString file = QFileDialog::getOpenFileName( this, "Add Existing Database", QDir::homePath(), "DB Files (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    addDBConnection( file );
  }
}

/*-------------------------------------------------------------*/

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

/*-------------------------------------------------------------*/

void GCMainWindow::setSessionDB( QString dbName )
{
  if( !m_dbInterface->setSessionDB( dbName ) )
  {
    QString error = QString( "Failed to set session \"%1\" as active - [%2]" ).arg( dbName )
                                                                              .arg( m_dbInterface->getLastError() );
    showErrorMessageBox( error );
  }
}

/*-------------------------------------------------------------*/

void GCMainWindow::removeDB()
{
  showSessionForm( true );
}

/*-------------------------------------------------------------*/


void GCMainWindow::removeDBConnection(QString dbName)
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

/*-------------------------------------------------------------*/

void GCMainWindow::switchDBSession()
{
  showSessionForm();
}

/*-------------------------------------------------------------*/

void GCMainWindow::addNewElement()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::deleteElement()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::addAsChild()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::addAsSibling()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::update()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::deleteElementFromDB()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::deleteAttributeValuesFromDB()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::revertDirectEdit()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::saveDirectEdit()
{

}

/*-------------------------------------------------------------*/

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

/*-------------------------------------------------------------*/

void GCMainWindow::showErrorMessageBox( const QString &errorMsg )
{
  QMessageBox::information( this, "Error!", errorMsg );
}

/*-------------------------------------------------------------*/
