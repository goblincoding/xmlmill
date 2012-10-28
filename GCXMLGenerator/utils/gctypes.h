#ifndef GCTYPES_H
#define GCTYPES_H

#include <QMap>
#include <QPair>
#include <QStringList>

/*-------------------------------------------------------------*/

/* The elements map consists of an DOM element name as key with a
  corresponding pair which consists of a list of associated attributes
  as well as a list of associated comments (XML comments that have been
  used in the past and are stored in the database). */
typedef QPair< QStringList /*attributes*/, QStringList /*comments*/ > GCElementPair;
typedef QMap < QString, GCElementPair > GCElementsMap;

/* The attributes map consists of a DOM attribute name as key
  with a corresponding list of known associated values. */
typedef QMap< QString, QStringList > GCAttributesMap;

/*-------------------------------------------------------------*/

#endif // GCTYPES_H
