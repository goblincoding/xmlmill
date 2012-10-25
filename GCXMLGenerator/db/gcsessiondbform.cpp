#include "gcsessiondbform.h"
#include "ui_gcsessiondbform.h"
#include <QFileDialog>

/*-------------------------------------------------------------*/

GCSessionDBForm::GCSessionDBForm( QStringList dbList, QWidget *parent ) :
  QDialog( parent),
  ui     ( new Ui::GCSessionDBForm )
{
  ui->setupUi( this );

  if( dbList.empty() )
  {
    ui->okButton->setEnabled( false );
    ui->comboBox->addItem( "No DB Connections yet, hit \"Add New\" below!" );
  }
  else
  {
    ui->comboBox->addItems( dbList );
  }

  connect( ui->addNewButton, SIGNAL( clicked() ), this, SLOT  ( addNew() ) );
  connect( ui->okButton,     SIGNAL( clicked() ), this, SLOT  ( open() ) );
  connect( ui->cancelButton, SIGNAL( clicked() ), this, SIGNAL( userCancelled() ) );
  connect( ui->cancelButton, SIGNAL( clicked() ), this, SLOT  ( close() ) );
}

/*-------------------------------------------------------------*/

GCSessionDBForm::~GCSessionDBForm()
{
  delete ui;
}

/*-------------------------------------------------------------*/

void GCSessionDBForm::open()
{
  emit dbSelected( ui->comboBox->currentText() );
  this->close();
}

/*-------------------------------------------------------------*/

void GCSessionDBForm::addNew()
{
  QString file = QFileDialog::getSaveFileName( this, "Add Database", QDir::homePath(), "DB Files (*.db)" );

  /* If the user clicked "OK". */
  if( !file.isEmpty() )
  {
    emit newConnection( file );
    this->close();
  }
}

/*-------------------------------------------------------------*/
