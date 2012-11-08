#include "gcmessagebox.h"

/*--------------------------------------------------------------------------------------*/

GCMessageBox::GCMessageBox( Query query, QWidget *parent ) :
  QMessageBox( parent ),
  m_query    ( query ),
  m_queryMap ()
{
  readUserPreferences();
  setupQueryMap();

  QueryStruct settings = m_queryMap.value( query );
  setText           ( settings.text );
  setInformativeText( settings.informativeText );
  setDetailedText   ( settings.detailedText );
  setStandardButtons( settings.standardButtons );
  setDefaultButton  ( settings.defaultButton );
  setIcon           ( settings.icon );
}

/*--------------------------------------------------------------------------------------*/

GCMessageBox::~GCMessageBox()
{
  // TODO: save user preferences here
}

/*--------------------------------------------------------------------------------------*/

void GCMessageBox::displayQuery()
{
  /* If the user doesn't want to see this message again, replace the map entry with
    the updated exclusion preference set to "true". */

}

/*--------------------------------------------------------------------------------------*/

void GCMessageBox::readUserPreferences()
{
  // TODO: read in user preferences from file.
}

/*--------------------------------------------------------------------------------------*/

void GCMessageBox::setupQueryMap()
{
  QueryStruct query;
  query.text = "Unknown XML Style";
  query.detailedText = "The current active database has no knowledge of the\n"
                       "specific XML style (the elements, attributes, attribute values and\n"
                       "all the associations between them) of the document you are trying to open.\n\n"
                       "You can either:\n\n"
                       "1. Select an existing database connection that describes this type of XML, or\n"
                       "2. Switch to \"Super User\" mode and open the file again to import it to the database.";

  query.icon = QMessageBox::Warning;


//  QString text;
//  QString informativeText;
//  QString detailedText;
//  QMessageBox::Icon icon;
//  QMessageBox::StandardButton  defaultButton;
//  QMessageBox::StandardButtons standardButtons;


  m_queryMap.insert( UnknownXMLStyle, query );
}

/*--------------------------------------------------------------------------------------*/
