#include "gcmessagedialog.h"
#include "ui_gcmessagedialog.h"

/*--------------------------------------------------------------------------------------*/

GCMessageDialog::GCMessageDialog(const QString &title, const QString &text, ButtonCombo buttons, Buttons defaultButton, Icon icon, QWidget *parent ) :
  QDialog( parent ),
  ui     ( new Ui::GCMessageDialog )
{
  ui->setupUi(this);
  ui->plainTextEdit->setPlainText( text );

  switch( buttons )
  {
    case YesNo:
      ui->acceptButton->setText( "Yes" );
      ui->rejectButton->setText( "No" );
      break;
    case OKCancel:
      ui->acceptButton->setText( "OK" );
      ui->rejectButton->setText( "Cancel" );
      break;
    case OKOnly:
      ui->acceptButton->setText( "OK" );
      ui->rejectButton->setVisible( false );
  }

  switch( defaultButton )
  {
    case Yes:
    case OK:
      ui->acceptButton->setDefault( true );
      break;
    case No:
    case Cancel:
      ui->rejectButton->setDefault( true );
  }

  switch( icon )
  {
    case Information:
      ui->iconButton->setIcon( style()->standardIcon( QStyle::SP_MessageBoxInformation ) );
      break;
    case Warning:
      ui->iconButton->setIcon( style()->standardIcon( QStyle::SP_MessageBoxWarning ) );
      break;
    case Critical:
      ui->iconButton->setIcon( style()->standardIcon( QStyle::SP_MessageBoxCritical ) );
      break;
    case Question:
      ui->iconButton->setIcon( style()->standardIcon( QStyle::SP_MessageBoxQuestion ) );
      break;
    case NoIcon:
    default:
      ui->iconButton->setIcon( QIcon() );
  }

  connect( ui->acceptButton, SIGNAL( clicked() ),       this, SLOT( accept() ) );
  connect( ui->rejectButton, SIGNAL( clicked() ),       this, SLOT( reject() ) );
  connect( ui->checkBox,     SIGNAL( toggled( bool ) ), this, SLOT( setRememberUserChoice( bool ) ) );

  setAttribute( Qt::WA_DeleteOnClose );
  setWindowTitle( title );
}

/*--------------------------------------------------------------------------------------*/

GCMessageDialog::~GCMessageDialog()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCMessageDialog::setRememberUserChoice( bool remember )
{
  emit rememberUserChoice( remember );
}

/*--------------------------------------------------------------------------------------*/
