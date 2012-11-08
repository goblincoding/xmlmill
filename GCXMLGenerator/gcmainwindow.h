#ifndef GCMAINWINDOW_H
#define GCMAINWINDOW_H

#include <QMainWindow>
#include <QDomDocument>
#include <QMap>
#include "db/gcknowndbform.h"

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
  void resetDOM();
  void processDOMDoc();
  void showKnownDBForm    ( GCKnownDBForm::Buttons buttons );
  void showErrorMessageBox( const QString &errorMsg );
  void addDBConnection    ( const QString &dbName );
  void populateTreeWidget ( const QDomElement &parentElement, QTreeWidgetItem *parentItem );
  void resetTableWidget();

  void toggleAddElementWidgets();

  Ui::GCMainWindow    *ui;
  GCDataBaseInterface *m_dbInterface;
  QDomDocument         m_domDoc;
  QString              m_currentXMLFileName;
  bool                 m_userCancelled;
  bool                 m_superUserMode;

  QMap< QTreeWidgetItem*, QDomElement > m_treeItemNodes;


private slots:
  void treeWidgetItemChanged     ( QTreeWidgetItem *item, int column );
  void treeWidgetItemActivated   ( QTreeWidgetItem *item, int column );
  void collapseOrExpandTreeWidget( bool checked );
  void switchSuperUserMode       ( bool super );
  void userCancelledKnownDBForm  ();

  /* XML file related. */
  void newXMLFile();
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
  void deleteElementFromDOM();
  void addChildElementToDOM();

  /* Edit XML store. */
  void deleteElementFromDB();
  void deleteAttributeValuesFromDB();

  /* Direct DOM edit. */
  void revertDirectEdit();
  void saveDirectEdit();

};

#endif // GCMAINWINDOW_H
