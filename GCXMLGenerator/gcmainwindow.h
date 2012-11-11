#ifndef GCMAINWINDOW_H
#define GCMAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include "db/gcknowndbform.h"

namespace Ui {
  class GCMainWindow;
}

class GCDataBaseInterface;
class QSignalMapper;
class QTreeWidgetItem;
class QTableWidgetItem;
class QComboBox;
class QDomDocument;
class QDomElement;
class QTimer;

class GCMainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit GCMainWindow( QWidget *parent = 0 );
  ~GCMainWindow();
  
private:
  QString scrollToAnchor  ( QDomElement element );
  void showKnownDBForm    ( GCKnownDBForm::Buttons buttons );
  void showErrorMessageBox( const QString &errorMsg );
  void addDBConnection    ( const QString &dbName );
  void populateTreeWidget ( const QDomElement &parentElement, QTreeWidgetItem *parentItem );
  void processDOMDoc();
  void resetDOM();
  void resetTableWidget();
  void startSaveTimer();

  void toggleAddElementWidgets();

  Ui::GCMainWindow    *ui;
  GCDataBaseInterface *m_dbInterface;
  QSignalMapper       *m_signalMapper;
  QDomDocument        *m_domDoc;
  QComboBox           *m_currentCombo;
  QTimer              *m_saveTimer;
  QString              m_currentXMLFileName;
  bool                 m_userCancelled;
  bool                 m_superUserMode;
  bool                 m_rootElementSet;

  QMap< QTreeWidgetItem*, QDomElement > m_treeItemNodes;
  QMap< QComboBox*, int/* table row*/ > m_comboBoxes;

private slots:
  void treeWidgetItemChanged     ( QTreeWidgetItem *item, int column );
  void treeWidgetItemActivated   ( QTreeWidgetItem *item, int column );
  void attributeNameChanged      ( QTableWidgetItem *item );
  void setCurrentComboBox        ( QComboBox *combo );
  void attributeValueChanged     ( QString value );
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
