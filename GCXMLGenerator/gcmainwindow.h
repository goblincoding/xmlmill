#ifndef GCMAINWINDOW_H
#define GCMAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include "forms/gcknowndbform.h"

/*--------------------------------------------------------------------------------------*/
/*
  All the code refers to "database" whereas all the user prompts reference "profiles". This
  is deliberate.  In reality, everything is persisted to SQLite database files, but a friend
  suggested that end users may be intimidated by the use of the word "database" (especially
  if they aren't necessarily technically inclined) and that "profiles" may be less scary :)
*/

/*--------------------------------------------------------------------------------------*/

namespace Ui {
  class GCMainWindow;
}

class GCDataBaseInterface;
class QSignalMapper;
class QTreeWidgetItem;
class QTableWidgetItem;
class QComboBox;
class QDomDocument;
class QSettings;
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

  void setCurrentComboBox        ( QWidget *combo );
  void collapseOrExpandTreeWidget( bool checked );
  void switchSuperUserMode       ( bool super );

  void userCancelledKnownDBForm();

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

  void importXMLToDatabase();

  /* DOM and DB. */
  void showNewElementForm();

  void deleteElementFromDOM();
  void addChildElementToDOM();

  void deleteElementFromDB();
  void deleteAttributeValuesFromDB();

  /* Direct DOM edit. */
  void revertDirectEdit();
  void saveDirectEdit();

  /* Receives new element information from "GCElementForm". */
  void addNewElement( const QString &element, const QStringList &attributes );

  /* Receives user preference regarding future displays of a specific message
    from "GCMessageDialog". */
  void rememberPreference( bool remember );
  void forgetAllMessagePreferences();
  
private:
  void addDBConnection    ( const QString &dbName );
  void showErrorMessageBox( const QString &errorMsg );
  void showKnownDBForm    ( GCKnownDBForm::Buttons buttons );

  void saveSetting( const QString &key, const QVariant &value );
  void setTextEditXML( const QDomElement &element );
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
  QSettings           *m_settings;
  QWidget             *m_currentCombo;
  QTimer              *m_saveTimer;
  QString              m_currentXMLFileName;
  QString              m_activeAttributeName;
  bool                 m_userCancelled;
  bool                 m_superUserMode;
  bool                 m_rootElementSet;
  bool                 m_wasTreeItemActivated;
  bool                 m_newElementWasAdded;
  bool                 m_rememberPreference;
  bool                 m_busyImporting;
  bool                 m_DOMTooLarge;

  QMap< QTreeWidgetItem*, QDomElement > m_treeItemNodes;
  QMap< QWidget*, int/* table row*/ >   m_comboBoxes;
  QMap< QString /*setting name*/, QVariant /*message*/ > m_messages;

};

#endif // GCMAINWINDOW_H
