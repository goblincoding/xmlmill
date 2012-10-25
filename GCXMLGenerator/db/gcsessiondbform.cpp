#include "gcsessiondbform.h"
#include "ui_gcsessiondbform.h"

/*-------------------------------------------------------------*/

GCSessionDBForm::GCSessionDBForm( QStringList dbList, QWidget *parent ) :
  QWidget( parent),
  ui     ( new Ui::GCSessionDBForm )
{
  ui->setupUi(this);
  ui->comboBox->addItems( dbList );

  connect( ui->addNewButton, SIGNAL( clicked() ), this, SLOT( addNew() ) );
  connect( ui->okButton,     SIGNAL( clicked() ), this, SLOT( open() ) );
  connect( ui->cancelButton, SIGNAL( clicked() ), this, SIGNAL( userCancelled() ) );
}

/*-------------------------------------------------------------*/

GCSessionDBForm::~GCSessionDBForm()
{
  delete ui;
}

/*-------------------------------------------------------------*/

void GCSessionDBForm::open()
{
  QString dbName( "" );

  emit dbSelected( dbName );
}

/*-------------------------------------------------------------*/

void GCSessionDBForm::addNew()
{
  QString dbName( "" );

  emit dbSelected( dbName );
}

/*-------------------------------------------------------------*/
