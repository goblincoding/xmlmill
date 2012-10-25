#ifndef GCMAINWINDOW_H
#define GCMAINWINDOW_H

#include <QMainWindow>
#include <QDomDocument>

namespace Ui {
  class GCMainWindow;
}

class GCDataBaseInterface;
class QTreeWidgetItem;

class GCMainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit GCMainWindow( QWidget *parent = 0 );
  ~GCMainWindow();
  
private:
  void showSessionForm();
  void showErrorMessageBox( QString errorMsg );
  void processInputXML();
  void populateTreeWidget( const QDomElement &parentElement, QTreeWidgetItem *parentItem );

  Ui::GCMainWindow    *ui;
  GCDataBaseInterface *m_dbInterface;
  QDomDocument         m_domDoc;
  QString              m_fileName;

  QMap< QString, QStringList > m_elementAttributes;
  QMap< QString, QStringList > m_attributeValues;

private slots:
  /* XML file related. */
  void openFile();
  void saveFile();

  /* Database related. */
  void addNewDB();
  void addExistingDB();
  void removeDB();
  void switchDBSession();

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
  void revertDirectEdit();
  void saveDirectEdit();
};

#endif // GCMAINWINDOW_H
