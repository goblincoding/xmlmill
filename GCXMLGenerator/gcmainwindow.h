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

private slots:
  void treeWidgetItemChanged     ( QTreeWidgetItem *item, int column );
  void treeWidgetItemActivated   ( QTreeWidgetItem *item, int column );

  void setActiveAttributeName    ( QTableWidgetItem *item );
  void attributeNameChanged      ( QTableWidgetItem *item );
  void attributeValueChanged     ( const QString &value );
  void attributeValueAdded       ( const QString &value );

  void setCurrentComboBox        ( QComboBox *combo );
  void collapseOrExpandTreeWidget( bool checked );
  void switchSuperUserMode       ( bool super );
  void userCancelledKnownDBForm  ();

  /* XML file related. */
  void newXMLFile();
  void openXMLFile();
  void saveXMLFile();
  void saveXMLFileAs();

  /* Database related. */
  void addNewDB();      // calls addDBConnection
  void addExistingDB(); // calls addDBConnection

  void switchDBSession();                     // shows known DB form
  void setSessionDB( const QString &dbName ); // receives signal from DB form

  void removeDB();                                  // shows known DB form
  void removeDBConnection( const QString &dbName ); // receives signal from DB form

  /* Build XML. */
  void deleteElementFromDOM();
  void addChildElementToDOM();

  /* Edit XML store. */
  void deleteElementFromDB();
  void deleteAttributeValuesFromDB();

  /* Direct DOM edit. */
  void revertDirectEdit();
  void saveDirectEdit();
  
private:
  QString scrollAnchorText( const QDomElement &element );
  void addDBConnection    ( const QString &dbName );
  void showErrorMessageBox( const QString &errorMsg );
  void showKnownDBForm    ( GCKnownDBForm::Buttons buttons );

  void resetTableWidget();
  void startSaveTimer();

  void toggleAddElementWidgets();

  void processDOMDoc();
  void populateTreeWidget( const QDomElement &parentElement, QTreeWidgetItem *parentItem );
  void resetDOM();

  Ui::GCMainWindow    *ui;
  GCDataBaseInterface *m_dbInterface;
  QSignalMapper       *m_signalMapper;
  QDomDocument        *m_domDoc;
  QComboBox           *m_currentCombo;
  QTimer              *m_saveTimer;
  QString              m_currentXMLFileName;
  QString              m_activeAttributeName;
  bool                 m_userCancelled;
  bool                 m_superUserMode;
  bool                 m_rootElementSet;
  bool                 m_wasTreeItemActivated;

  QMap< QTreeWidgetItem*, QDomElement > m_treeItemNodes;
  QMap< QComboBox*, int/* table row*/ > m_comboBoxes;

};

#endif // GCMAINWINDOW_H
