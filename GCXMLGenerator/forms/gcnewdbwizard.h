#ifndef GCNEWDBWIZARD_H
#define GCNEWDBWIZARD_H

#include <QWizard>

namespace Ui
{
  class GCNewDBWizard;
}

/*------------------------------------------------------------------------------------------

  A wizard to guide users through the process of creating and populating new databases.

------------------------------------------------------------------------------------------*/

class GCNewDBWizard : public QWizard
{
  Q_OBJECT
  
public:
  explicit GCNewDBWizard( QWidget *parent = 0 );
  ~GCNewDBWizard();
  QString xmlFileName();
  QString dbFileName();

public slots:
  void accept();
  
private slots:
  void openDBFileDialog();
  void openXMLFileDialog();

private:
  Ui::GCNewDBWizard *ui;
  QString m_dbFileName;
  QString m_xmlFileName;
};

#endif // GCNEWDBWIZARD_H
