#ifndef GCMAINWINDOW_H
#define GCMAINWINDOW_H

#include <QMainWindow>

namespace Ui {
  class GCMainWindow;
}

class GCDataBaseInterface;

class GCMainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit GCMainWindow( QWidget *parent = 0 );
  ~GCMainWindow();
  
private:
  void showSessionForm();

  Ui::GCMainWindow    *ui;
  GCDataBaseInterface *m_dbInterface;

private slots:
  /* XML file related. */
  void newFile();
  void openFile();
  void saveFile();

  /* Database related. */
  void newDB();
  void existingDB();
  void removeDB();
  void switchDB();

  /* Build XML. */
  void addNewElement();
  void deleteElement();
  void addAsChild();
  void addAsSibling();

  /* Edit XML store. */
  void update();
  void deleteElementFromDB();
  void deleteAttributeValuesFromDB();

  /* Direct DOM edit. */
  void revert();
  void saveChanges();
};

#endif // GCMAINWINDOW_H
