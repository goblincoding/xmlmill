#ifndef GCTYPES_H
#define GCTYPES_H

#include <QMap>
#include <QStringList>

/*-------------------------------------------------------------*/

/* The elements map consists of an DOM element name as key with a
  corresponding list of associated attributes. */
typedef QMap< QString, QStringList > GCElementsMap;

/* The attributse map consists of a DOM attribute name as key
  with a corresponding list of associated values. */
typedef QMap< QString, QStringList > GCAttributesMap;

/*-------------------------------------------------------------*/

#endif // GCTYPES_H
