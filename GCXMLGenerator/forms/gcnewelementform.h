#ifndef GCNEWELEMENTFORM_H
#define GCNEWELEMENTFORM_H

#include <QWidget>

namespace Ui {
  class GCNewElementForm;
}

class GCNewElementForm : public QWidget
{
  Q_OBJECT
  
public:
  explicit GCNewElementForm( QWidget *parent = 0 );
  ~GCNewElementForm();

signals:
  void newElementDetails( const QString&, const QStringList& );
  
private slots:
  void addElementAndAttributes();
  void showHelp();

private:
  Ui::GCNewElementForm *ui;
};

#endif // GCNEWELEMENTFORM_H
