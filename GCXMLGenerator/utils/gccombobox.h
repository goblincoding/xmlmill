#ifndef GCCOMBOBOX_H
#define GCCOMBOBOX_H

#include <QComboBox>

/*--------------------------------------------------------------------------------------*/
/*
  The only reason this class exists is so that we may know when a combo box is activated.
  Initially I understood that the "activated" signal is emitted when a user clicks on
  a QComboBox (e.g. when the dropdown is expanded), but it turns out that this is not the
  case.
*/

/*--------------------------------------------------------------------------------------*/

class GCComboBox : public QComboBox
{
  Q_OBJECT
public:
  explicit GCComboBox( QWidget *parent = 0 );
  
protected:
  void mousePressEvent( QMouseEvent *e );
  
};

#endif // GCCOMBOBOX_H
