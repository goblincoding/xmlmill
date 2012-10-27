#include "gcsessiondbform.h"
#include "ui_gcsessiondbform.h"
#include <QFileDialog>

/*-------------------------------------------------------------*/

GCSessionDBForm::GCSessionDBForm( QStringList dbList, bool remove, QWidget *parent ) :
  QDialog ( parent),
  ui      ( new Ui::GCSessionDBForm ),
  m_remove( remove )
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

  if( m_remove )
  {
    ui->addNewButton->setVisible( false );
  }

  connect( ui->addNewButton, SIGNAL( clicked() ), this, SLOT  ( addNew() ) );
  connect( ui->okButton,     SIGNAL( clicked() ), this, SLOT  ( select() ) );
  connect( ui->cancelButton, SIGNAL( clicked() ), this, SIGNAL( userCancelled() ) );
  connect( ui->cancelButton, SIGNAL( clicked() ), this, SLOT  ( close() ) );
}

/*-------------------------------------------------------------*/

GCSessionDBForm::~GCSessionDBForm()
{
  delete ui;
}

/*-------------------------------------------------------------*/

void GCSessionDBForm::select()
{
  if( m_remove )
  {
    emit dbRemoved( ui->comboBox->currentText() );
  }
  else
  {
    emit dbSelected( ui->comboBox->currentText() );
  }

  this->close();
}

/*-------------------------------------------------------------*/

void GCSessionDBForm::addNew()
{
  emit newConnection();
  this->close();
}

/*-------------------------------------------------------------*/
