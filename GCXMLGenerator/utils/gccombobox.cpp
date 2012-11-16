#include "gccombobox.h"

GCComboBox::GCComboBox( QWidget *parent ) :
  QComboBox( parent )
{
}

/*--------------------------------------------------------------------------------------*/

void GCComboBox::mousePressEvent( QMouseEvent *e )
{
  QComboBox::mousePressEvent( e );
  emit activated( currentIndex() );
}

/*--------------------------------------------------------------------------------------*/
