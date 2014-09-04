/* Copyright (c) 2012 - 2013 by William Hallatt.
 *
 * This file forms part of "XML Mill".
 *
 * The official website for this project is <http://www.goblincoding.com> and,
 * although not compulsory, it would be appreciated if all works of whatever
 * nature using this source code (in whole or in part) include a reference to
 * this site.
 *
 * Should you wish to contact me for whatever reason, please do so via:
 *
 *                 <http://www.goblincoding.com/contact>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#include "batchprocesshelper.h"

#include <QDomDocument>

/*--------------------------------- MEMBER FUNCTIONS ----------------------------------*/

BatchProcessorHelper::BatchProcessorHelper( const QDomDocument* domDoc,
                                                const QString& stringListSeparator,
                                                const QStringList& knownElements,
                                                const QStringList& knownAttributes )
: m_stringListSeparator       ( stringListSeparator ),
  m_knownElements             ( knownElements ),
  m_knownAttributeKeys        ( knownAttributes ),
  m_newElementsToAdd          (),
  m_newElementChildrenToAdd   (),
  m_newElementAttributesToAdd (),
  m_elementsToUpdate          (),
  m_elementChildrenToUpdate   (),
  m_elementAttributesToUpdate (),
  m_newAttributeKeysToAdd     (),
  m_newAssociatedElementsToAdd(),
  m_newAttributeValuesToAdd   (),
  m_attributeKeysToUpdate     (),
  m_associatedElementsToUpdate(),
  m_attributeValuesToUpdate   (),
  m_unsorted                  (),
  m_records                   ()
{
  QDomElement root = domDoc->documentElement();
  createRecord( root );
  processElement( root );   // kicks off a chain of recursive DOM element traversals
  sortRecords();
  createVariantLists();
}

/*--------------------------------------------------------------------------------------*/

void BatchProcessorHelper::processElement( const QDomElement& parentElement )
{
  QDomElement element = parentElement.firstChildElement();

  while( !element.isNull() )
  {
    createRecord( element );
    processElement( element );
    element = element.nextSiblingElement();
  }
}

/*--------------------------------------------------------------------------------------*/

void BatchProcessorHelper::createRecord( const QDomElement& element )
{
  ElementRecord record;

  /* Stick the attributes and their corresponding values into the record map. */
  QDomNamedNodeMap attributeNodes = element.attributes();

  for( int i = 0; i < attributeNodes.size(); ++i )
  {
    QDomAttr attribute = attributeNodes.item( i ).toAttr();

    if( !attribute.isNull() )
    {
      record.attributes.insert( attribute.name(), QStringList( attribute.value() ) );
    }
  }

  /* Collect all the first level (element) children associated with this element. */
  QDomNodeList children = element.childNodes();

  for( int i = 0; i < children.size(); ++i )
  {
    QDomNode child = children.at( i );

    if( child.isElement() )
    {
      record.children.append( child.toElement().tagName() );
    }
  }

  /* We'll sort it all later, for now get the recursive calls out of the way. */
  m_unsorted.insert( element.tagName(), record );
}

/*--------------------------------------------------------------------------------------*/

void BatchProcessorHelper::sortRecords()
{
  /* We inserted every element in the DOM doc into the unsorted map and will almost
    definitely have duplicates.  Since the DB requires unique element entries (elements
    are the primary keys), we iterate through the unique keys and consolidate the entries
    per element. */
  foreach( QString element, m_unsorted.uniqueKeys() )
  {
   /* Retrieve all the records associated with this element. */
   QList< ElementRecord > duplicateRecords = m_unsorted.values( element );

   /* If we have more than one record for the same element, then we need to consolidate
     all the attributes associated with this particular element. */
   if( duplicateRecords.size() > 1 )
   {
     ElementRecord record;

     /* Just stick all the attributes and their values into a map for now
       so that we don't have to run the consolidation functionality on
       each iteration...that would just be silly... */
     QMultiMap< QString, QStringList > recordAttributes;

     for( int i = 0; i < duplicateRecords.size(); ++i )
     {
       recordAttributes += duplicateRecords.at( i ).attributes;

       /* Don't consolidate the attribute values here (see above comment) but since
         we're already iterating through the list, we may as well consolidate the
         children while we're at it (they are relatively simple to handle compared
         to the attribute value maps). */
       record.children.append( duplicateRecords.at( i ).children );
     }

     record.children.removeDuplicates();

     /* Now we can sort out the chaos. For each unique attribute, we will obtain
       the associated QStringList(s) of attribute values and consolidate the lot. */
     foreach( QString attribute, recordAttributes.uniqueKeys() )
     {
      QList< QStringList > attributeValues = recordAttributes.values( attribute );
      QStringList finalListOfAttributeValues;

      for( int j = 0; j < attributeValues.size(); ++j )
      {
        finalListOfAttributeValues.append( attributeValues.at( j ) );
      }

      finalListOfAttributeValues.removeDuplicates();
      record.attributes.insert( attribute, finalListOfAttributeValues );
     }

     m_records.insert( element, record );
   }
   else
   {
     /* If we only have one entry for this particular element name, we can
       safely insert it into the sorted map. */
     ElementRecord record = duplicateRecords.at( 0 );
     record.children.removeDuplicates();
     m_records.insert( element, record );
   }
  }
}

/*--------------------------------------------------------------------------------------*/

void BatchProcessorHelper::createVariantLists()
{
  /* First see which of the records we created from the DOM doc are completely new
    and which ones we have prior knowledge of. */
  QList< QString > elementNames = m_records.keys();

  for( int i = 0; i < elementNames.size(); ++i )
  {
    if( !m_knownElements.contains( elementNames.at( i ) ) )
    {
      m_newElementsToAdd << elementNames.at( i );
    }
    else
    {
      m_elementsToUpdate << elementNames.at( i );
    }
  }

  /* Deal with all the new elements first. */
  foreach( QVariant var, m_newElementsToAdd )
  {
   QString element = var.toString();

   /* Do we have first level children? */
   if( !m_records.value( element ).children.isEmpty() )
   {
     m_newElementChildrenToAdd << m_records.value( element ).children.join( m_stringListSeparator );
   }
   else
   {
     /* To keep the indices in sync, the following will result in a NULL value
       being bound to the relevant prepared query on the DB side (we need to have
       exactly the same number of items in all lists associated with the "new elements"
       in order for the batch bind to succeed). The same argument holds for the
       "elements to update" and two types of attribute lists below. */
     m_newElementChildrenToAdd << QVariant( QVariant::String );
   }

   /* Do we have associated attributes? */
   if( !m_records.value( element ).attributes.keys().isEmpty() )
   {
     QStringList attributeNames = m_records.value( element ).attributes.keys();
     m_newElementAttributesToAdd << attributeNames.join( m_stringListSeparator );
   }
   else
   {
     /* See comment for "m_newElementChildrenToAdd" above.*/
     m_newElementAttributesToAdd << QVariant( QVariant::String );
   }
  }

  /* Now we deal with all the elements that will have to be updated. */
  foreach( QVariant var, m_elementsToUpdate )
  {
   QString element = var.toString();

   /* Do we have first level children? */
   if( !m_records.value( element ).children.isEmpty() )
   {
     m_elementChildrenToUpdate << m_records.value( element ).children.join( m_stringListSeparator );
   }
   else
   {
     /* See comment for "m_newElementChildrenToAdd" above.*/
     m_elementChildrenToUpdate << QVariant( QVariant::String );
   }

   /* Do we have associated attributes? */
   if( !m_records.value( element ).attributes.keys().isEmpty() )
   {
     QStringList attributeNames = m_records.value( element ).attributes.keys();
     m_elementAttributesToUpdate << attributeNames.join( m_stringListSeparator );
   }
   else
   {
     /* See comment for "m_newElementChildrenToAdd" above.*/
     m_elementAttributesToUpdate << QVariant( QVariant::String );
   }
  }

  /* Separate the new attribute keys and associated values from the existing ones. */
  foreach( QString element, elementNames )
  {
    QList< QString > attributeNames = m_records.value( element ).attributes.keys();

    foreach( QString attribute, attributeNames )
    {
      QStringList attributeValues = m_records.value( element ).attributes.value( attribute );
      attributeValues.removeDuplicates();

      /* Do we know about this attribute? (by the way, the "!" is a separator
        used to create a unique string name from the element and associated
        attribute for ease of comparison with the attribute keys list we get
        given...this is not ideal, but the only solution I have at the moment. */
      if( !m_knownAttributeKeys.contains( attribute + "!" + element ) )
      {
        m_newAttributeKeysToAdd << attribute;
        m_newAssociatedElementsToAdd << element;

        if( !attributeValues.isEmpty() )
        {
          m_newAttributeValuesToAdd << attributeValues.join( m_stringListSeparator );
        }
        else
        {
          /* See comment for "m_newElementChildrenToAdd" above.*/
          m_newAttributeValuesToAdd << QVariant( QVariant::String );
        }
      }
      else
      {
        m_attributeKeysToUpdate << attribute;
        m_associatedElementsToUpdate << element;

        if( !attributeValues.isEmpty() )
        {
          m_attributeValuesToUpdate << attributeValues.join( m_stringListSeparator );
        }
        else
        {
          /* See comment for "m_newElementChildrenToAdd" above.*/
          m_attributeValuesToUpdate << QVariant( QVariant::String );
        }
      }
    }
  }
}

/*--------------------------------------------------------------------------------------*/

const QVariantList& BatchProcessorHelper::newElementsToAdd() const
{
  return m_newElementsToAdd;
}

/*--------------------------------------------------------------------------------------*/

const QVariantList& BatchProcessorHelper::newElementChildrenToAdd() const
{
  return m_newElementChildrenToAdd;
}

/*--------------------------------------------------------------------------------------*/

const QVariantList& BatchProcessorHelper::newElementAttributesToAdd() const
{
  return m_newElementAttributesToAdd;
}

/*--------------------------------------------------------------------------------------*/

const QVariantList& BatchProcessorHelper::elementsToUpdate() const
{
  return m_elementsToUpdate;
}

/*--------------------------------------------------------------------------------------*/

const QVariantList& BatchProcessorHelper::elementChildrenToUpdate() const
{
  return m_elementChildrenToUpdate;
}

/*--------------------------------------------------------------------------------------*/

const QVariantList& BatchProcessorHelper::elementAttributesToUpdate() const
{
  return m_elementAttributesToUpdate;
}

/*--------------------------------------------------------------------------------------*/

const QVariantList& BatchProcessorHelper::newAttributeKeysToAdd() const
{
  return m_newAttributeKeysToAdd;
}

/*--------------------------------------------------------------------------------------*/

const QVariantList& BatchProcessorHelper::newAssociatedElementsToAdd() const
{
  return m_newAssociatedElementsToAdd;
}
/*--------------------------------------------------------------------------------------*/

const QVariantList& BatchProcessorHelper::newAttributeValuesToAdd() const
{
  return m_newAttributeValuesToAdd;
}

/*--------------------------------------------------------------------------------------*/

const QVariantList& BatchProcessorHelper::attributeKeysToUpdate() const
{
  return m_attributeKeysToUpdate;
}

/*--------------------------------------------------------------------------------------*/

const QVariantList& BatchProcessorHelper::associatedElementsToUpdate() const
{
  return m_associatedElementsToUpdate;
}

/*--------------------------------------------------------------------------------------*/

const QVariantList& BatchProcessorHelper::attributeValuesToUpdate() const
{
  return m_attributeValuesToUpdate;
}

/*--------------------------------------------------------------------------------------*/
