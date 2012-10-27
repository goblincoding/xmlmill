#ifndef GCMAINWINDOW_H
#define GCMAINWINDOW_H

#include "utils/gctypes.h"
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
  void showSessionForm( bool remove = false );
  void showErrorMessageBox( const QString &errorMsg );
  void processInputXML();
  void populateTreeWidget( const QDomElement &parentElement, QTreeWidgetItem *parentItem );
  void populateMaps( const QDomElement &element );
  void updateDataBase();
  void addDBConnection( const QString &dbName );

  Ui::GCMainWindow    *ui;
  GCDataBaseInterface *m_dbInterface;
  GCElementsMap        m_elements;
  GCAttributesMap      m_attributes;
  QDomDocument         m_domDoc;
  QString              m_fileName;


private slots:
  /* XML file related. */
  void openFile();
  void saveFile();
  void saveFileAs();

  /* Database related. */
  void addNewDB();
  void addExistingDB();
  void switchDBSession();
  void removeDB();
  void setSessionDB( QString dbName );
  void removeDBConnection( QString dbName );

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
