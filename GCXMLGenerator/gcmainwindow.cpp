#include "gcmainwindow.h"
#include "ui_gcmainwindow.h"
#include "db/gcdatabaseinterface.h"
#include "db/gcsessiondbform.h"
#include <QMessageBox>

/*-------------------------------------------------------------*/

GCMainWindow::GCMainWindow( QWidget *parent ) :
  QMainWindow   ( parent ),
  ui            ( new Ui::GCMainWindow ),
  m_dbInterface ( new GCDataBaseInterface )
{
  ui->setupUi( this );

  /* XML File related. */
  connect( ui->actionNew,  SIGNAL( triggered() ), this, SLOT( newFile() ) );
  connect( ui->actionOpen, SIGNAL( triggered() ), this, SLOT( openFile() ) );
  connect( ui->actionSave, SIGNAL( triggered() ), this, SLOT( saveFile() ) );

  /* Database related. */
  connect( ui->actionAddNewDatabase,        SIGNAL( triggered() ), this, SLOT( newDB() ) );
  connect( ui->actionAddExistingDatabase,   SIGNAL( triggered() ), this, SLOT( existingDB() ) );
  connect( ui->actionRemoveDatabase,        SIGNAL( triggered() ), this, SLOT( removeDB() ) );
  connect( ui->actionSwitchSessionDatabase, SIGNAL( triggered() ), this, SLOT( switchDB() ) );

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
  connect( ui->dockWidgetRevertButton, SIGNAL( clicked() ), this, SLOT( revert() ) );
  connect( ui->dockWidgetSaveButton,   SIGNAL( clicked() ), this, SLOT( saveChanges() ) );

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

void GCMainWindow::newFile()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::openFile()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::saveFile()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::newDB()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::existingDB()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::removeDB()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::switchDB()
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

void GCMainWindow::revert()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::saveChanges()
{

}

/*-------------------------------------------------------------*/
