#include "gcmainwindow.h"
#include "ui_gcmainwindow.h"

/*-------------------------------------------------------------*/

GCMainWindow::GCMainWindow( QWidget *parent ) :
  QMainWindow( parent ),
  ui         ( new Ui::GCMainWindow )
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
  connect( ui->editXMLDeleteAttributesButton, SIGNAL( clicked() ), this, SLOT( deleteAttributeValues() ) );
  connect( ui->editXMLDeleteElementsButton,   SIGNAL( clicked() ), this, SLOT( deleteElement() ) );

  /* Direct DOM edit. */
  connect( ui->dockWidgetRevertButton, SIGNAL( clicked() ), this, SLOT( revert() ) );
  connect( ui->dockWidgetSaveButton,   SIGNAL( clicked() ), this, SLOT( saveChanges() ) );
}

/*-------------------------------------------------------------*/

GCMainWindow::~GCMainWindow()
{
  delete ui;
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

void GCMainWindow::deleteElement()
{

}

/*-------------------------------------------------------------*/

void GCMainWindow::deleteAttributeValues()
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
