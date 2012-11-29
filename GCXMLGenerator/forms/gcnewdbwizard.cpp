#include "gcnewdbwizard.h"
#include "ui_gcnewdbwizard.h"

#include <QFileDialog>

/*--------------------------------------------------------------------------------------*/

GCNewDBWizard::GCNewDBWizard( QWidget *parent ) :
  QWizard      ( parent ),
  ui           ( new Ui::GCNewDBWizard ),
  m_dbFileName ( "" ),
  m_xmlFileName( "" )
{
  ui->setupUi(this);
  connect( ui->importFileButton, SIGNAL( clicked() ), this, SLOT( openXMLFileDialog() ) );
  connect( ui->newDBButton,      SIGNAL( clicked() ), this, SLOT( openDBFileDialog() ) );
}

/*--------------------------------------------------------------------------------------*/

GCNewDBWizard::~GCNewDBWizard()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

QString GCNewDBWizard::dbFileName()
{
  return m_dbFileName;
}

/*--------------------------------------------------------------------------------------*/

QString GCNewDBWizard::xmlFileName()
{
  return m_xmlFileName;
}

/*--------------------------------------------------------------------------------------*/

void GCNewDBWizard::accept()
{
  m_xmlFileName = ui->importFileLineEdit->text();
  m_dbFileName  = ui->newDBLineEdit->text();

  QDialog::accept();
}

/*--------------------------------------------------------------------------------------*/

void GCNewDBWizard::openDBFileDialog()
{
  QString fileName = QFileDialog::getSaveFileName( this, "Add New Profile", QDir::homePath(), "Profile Files (*.db)" );

  /* If the user clicked "OK", continue (a cancellation will result in an empty file name). */
  if( !fileName.isEmpty() )
  {
    ui->newDBLineEdit->setText( fileName );
  }
}

/*--------------------------------------------------------------------------------------*/

void GCNewDBWizard::openXMLFileDialog()
{
  QString fileName = QFileDialog::getOpenFileName( this, "Open XML File", QDir::homePath(), "XML Files (*.*)" );

  /* If the user clicked "OK", continue (a cancellation will result in an empty file name). */
  if( !fileName.isEmpty() )
  {
    ui->importFileLineEdit->setText( fileName );
  }
}

/*--------------------------------------------------------------------------------------*/
