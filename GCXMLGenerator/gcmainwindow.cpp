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
  connect( ui->actionOpen, SIGNAL( triggered() ), this, SLOT( openFile() ) );
  connect( ui->actionSave, SIGNAL( triggered() ), this, SLOT( saveFile() ) );

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
    connection for this session. Since we trigger the DB interface methods via signals,
    we won't know if they returned successfully unless we query the last error message. */
  showSessionForm();

  if( m_dbInterface->getLastError() != "" )
  {
    QString errorMsg = QString( "Something terrible happened and we can't fix it: %1" ).arg( m_dbInterface->getLastError() );
    QMessageBox::critical( this, "Error!", errorMsg );
    this->close();
  }
}

/*-------------------------------------------------------------*/

GCMainWindow::~GCMainWindow()
{
  delete m_dbInterface;
  delete ui;
}

/*-------------------------------------------------------------*/

void GCMainWindow::showSessionForm()
{
  GCSessionDBForm *sessionForm = new GCSessionDBForm( m_dbInterface->getDBList(), this );

  connect( sessionForm,   SIGNAL( userCancelled() ),          this,          SLOT  ( close() ) );
  connect( sessionForm,   SIGNAL( dbSelected   ( QString ) ), m_dbInterface, SLOT  ( setSessionDB( QString ) ) );
  connect( sessionForm,   SIGNAL( newConnection( QString ) ), m_dbInterface, SLOT  ( addDatabase ( QString ) ) );

  sessionForm->show();
}

/*-------------------------------------------------------------*/

void GCMainWindow::showErrorMessageBox( const QString &errorMsg )
{
  QMessageBox::information( this, "Error!", errorMsg );
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

void GCMainWindow::updateDataBase()
{
  if( !m_dbInterface->addElements( m_elements ) )
  {
    QString errMsg = QString( "Failed to update elements database: %1." ).arg( m_dbInterface->getLastError() );
    showErrorMessageBox( errMsg );
  }

  if( !m_dbInterface->addAttributes( m_attributes ) )
  {
    QString errMsg = QString( "Failed to update attributes database: %1." ).arg( m_dbInterface->getLastError() );
    showErrorMessageBox( errMsg );
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
        QString errorMsg = QString( "XML is broken: %1 (line: %2, column %3)." ).arg( xmlErr ).arg( line ).arg( col );
        showErrorMessageBox( errorMsg );
      }
    }
    else
    {
      QString errorMsg = QString( "Failed to open file: %1" ).arg( file.errorString() );
      showErrorMessageBox( errorMsg );
    }
  }
}

/*-------------------------------------------------------------*/

void GCMainWindow::saveFile()
{
  QString file = QFileDialog::getSaveFileName( this, "New File", QDir::homePath(), "XML Files (*.xml)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    m_fileName = file;
  }
}

/*-------------------------------------------------------------*/

void GCMainWindow::addNewDB()
{
  QString file = QFileDialog::getSaveFileName( this, "New Database", QDir::homePath(), "DB Files (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    if( !m_dbInterface->addDatabase( file ) )
    {
      QString error = QString( "Failed to add DB connection: %1" ).arg( m_dbInterface->getLastError() );
      showErrorMessageBox( error );
    }
  }
}

/*-------------------------------------------------------------*/

void GCMainWindow::addExistingDB()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::removeDB()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::switchDBSession()
{

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
