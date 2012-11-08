#ifndef GCMESSAGEBOX_H
#define GCMESSAGEBOX_H

#include <QMessageBox>
#include <QMap>

class GCMessageBox : public QMessageBox
{
  Q_OBJECT
public:
  enum Query
  {
    UnknownXMLStyle
  };

  explicit GCMessageBox( Query query, QWidget *parent = 0 );
  ~GCMessageBox();
  
signals:
  
public slots:

private:
  struct QueryStruct
  {
    QString text;
    QString informativeText;
    QString detailedText;
    bool    excluded;
    QMessageBox::Icon icon;
    QMessageBox::StandardButton  defaultButton;
    QMessageBox::StandardButtons standardButtons;

    QueryStruct() :
      text           ( "" ),
      informativeText( "" ),
      detailedText   ( "" ),
      excluded       ( false ),
      icon           ( QMessageBox::NoIcon ),
      defaultButton  ( QMessageBox::Ok ),
      standardButtons( QMessageBox::Ok )
    {}
  };

  void setupQueryMap();
  void readUserPreferences();
  void displayQuery();

  Query m_query;
  QMap< Query, QueryStruct > m_queryMap;
  
};

#endif // GCMESSAGEBOX_H
