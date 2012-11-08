#include "gcquerydialog.h"
#include "ui_gcquerydialog.h"

/*--------------------------------------------------------------------------------------*/

GCQueryDialog::GCQueryDialog( QWidget *parent ) :
  QDialog( parent ),
  ui     ( new Ui::GCQueryDialog )
{
  ui->setupUi(this);
}

/*--------------------------------------------------------------------------------------*/

GCQueryDialog::~GCQueryDialog()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/
