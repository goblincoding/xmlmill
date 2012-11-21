#ifndef GCHELP_H
#define GCHELP_H

#include <QObject>

/*--------------------------------------------------------------------------------------*/

/*
  This class will show the "Help" messages accessible from the main window.  Although it
  does somewhat complicate matters sticking the help messages into a separate class, it
  helps to declutter the (already large) main window class.
*/

/*--------------------------------------------------------------------------------------*/

class GCHelp : public QObject
{
  Q_OBJECT
public:
  explicit GCHelp(QObject *parent = 0);
  
signals:
  
public slots:
  
};

#endif // GCHELP_H
