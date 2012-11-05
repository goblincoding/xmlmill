#ifndef GCMAINWINDOW_H
#define GCMAINWINDOW_H

#include <QMainWindow>
#include <QDomDocument>
#include <QMap>

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
  void processDOMDoc();
  void batchUpsertDB();   // upsert - "update and insert"
  void populateTreeWidget ( const QDomElement &parentElement, QTreeWidgetItem *parentItem );
  void addDBConnection    ( const QString &dbName );
  void showKnownDBForm    ( bool remove = false );
  void showErrorMessageBox( const QString &errorMsg );

  Ui::GCMainWindow    *ui;
  GCDataBaseInterface *m_dbInterface;
  QDomDocument         m_domDoc;
  QString              m_currentXMLFileName;

  QMap< QTreeWidgetItem*, QDomElement > m_treeItemNodes;


private slots:
  void treeWidgetItemChanged  ( QTreeWidgetItem *item, int column );
  void treeWidgetItemActivated( QTreeWidgetItem *item, int column );
  void collapseOrExpandTreeWidget( bool checked );

  /* XML file related. */
  void openXMLFile();
  void saveXMLFile();
  void saveXMLFileAs();

  /* Database related. */
  void addNewDB();                            // calls addDBConnection
  void addExistingDB();                       // calls addDBConnection

  void switchDBSession();                     // shows known DB form
  void setSessionDB( QString dbName );        // receives signal from DB form

  void removeDB();                            // shows known DB form
  void removeDBConnection( QString dbName );  // receives signal from DB form
  void updateDataBase();

  /* Build XML. */
  void deleteElement();
  void addAsChild();
  void addAsSibling();

  /* Edit XML store. */
  void deleteElementFromDB();
  void deleteAttributeValuesFromDB();

  /* Direct DOM edit. */
  void revertDirectEdit();
  void saveDirectEdit();

};

#endif // GCMAINWINDOW_H
