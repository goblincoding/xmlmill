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
    ui->okButton->setVisible( false );
    ui->comboBox->addItem( "Let there be databases! (i.e. you'll have to add some)" );
  }
  else
  {
    ui->comboBox->addItems( dbList );
  }

  if( m_remove )
  {
    ui->addNewButton->setVisible( false );
    ui->addExistingButton->setVisible( false );
  }

  connect( ui->addNewButton,      SIGNAL( clicked() ), this, SIGNAL( newConnection() ) );
  connect( ui->addNewButton,      SIGNAL( clicked() ), this, SLOT  ( close() ) );

  connect( ui->addExistingButton, SIGNAL( clicked() ), this, SIGNAL( existingConnection() ) );
  connect( ui->addExistingButton, SIGNAL( clicked() ), this, SLOT  ( close() ) );

  connect( ui->cancelButton,      SIGNAL( clicked() ), this, SIGNAL( userCancelled() ) );
  connect( ui->cancelButton,      SIGNAL( clicked() ), this, SLOT  ( close() ) );

  connect( ui->okButton,          SIGNAL( clicked() ), this, SLOT  ( select() ) );
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
