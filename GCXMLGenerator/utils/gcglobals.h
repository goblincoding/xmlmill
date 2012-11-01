#ifndef GCGLOBALS_H
#define GCGLOBALS_H

#include <QString>

/*--------------------------------------------------------------------------------------*/

/* The database tables have fields containing strings of strings. For example, the
  "xmlelements" table maps a unique element against a list of all associated attribute
  values.  Since these values have to be entered into a single record, the easiest way
  is to insert a single (possibly massive) string containing all the associated attributes.
  To ensure that we can later extract the individual attributes again, we separate them with
  a sequence that should (theoretically) never be encountered.  This is that sequence. */
static const QString SEPARATOR( "~!@" );

/*--------------------------------------------------------------------------------------*/

#endif // GCGLOBALS_H
