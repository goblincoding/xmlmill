#include "gcnewelementform.h"
#include "ui_gcnewelementform.h"
#include <QMessageBox>

/*--------------------------------------------------------------------------------------*/

GCNewElementForm::GCNewElementForm( QWidget *parent ) :
  QWidget( parent ),
  ui     ( new Ui::GCNewElementForm )
{
  ui->setupUi( this );

  connect( ui->addPushButton,  SIGNAL( clicked() ), this, SLOT( addElementAndAttributes() ) );
  connect( ui->donePushButton, SIGNAL( clicked() ), this, SLOT( close() ) );
  connect( ui->helpToolButton, SIGNAL( clicked() ), this, SLOT( showHelp() ) );

  setAttribute( Qt::WA_DeleteOnClose );
  setWindowFlags( Qt::Dialog );
}

/*--------------------------------------------------------------------------------------*/

GCNewElementForm::~GCNewElementForm()
{
  delete ui;
}

/*--------------------------------------------------------------------------------------*/

void GCNewElementForm::addElementAndAttributes()
{
  QString element = ui->lineEdit->text();

  if( !element.isEmpty() )
  {
    QStringList attributes = ui->plainTextEdit->toPlainText().split( "\n" );
    emit newElementDetails( element, attributes );
  }

  ui->lineEdit->clear();
  ui->plainTextEdit->clear();
}

/*--------------------------------------------------------------------------------------*/

void GCNewElementForm::showHelp()
{
  QMessageBox::information( this,
                            "How this works...",
                            "Enter the name of the new element in the \"Element Name\"\n"
                            "field (no surprises there) and the attributes associated\n"
                            "with the element in the \"Attribute Names\" text area (again\n"
                            "pretty obvious).\n\n"
                            "What you might not know, however, is that although you can only add\n"
                            "one element at a time, you can add all the attributes for the\n"
                            "element in one go: simply stick each of them on a separate line\n"
                            "in the text edit area and hit \"Add\" when you're done...easy!\n\n"
                            "(oh, and if the element doesn't have associated attributes, just\n"
                            "leave the text edit area empty)");
}

/*--------------------------------------------------------------------------------------*/
