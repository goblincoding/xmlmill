#include "gcbatchprocessorhelper.h"
#include "utils/gcglobals.h"

/*--------------------------- NON-MEMBER UTILITY FUNCTIONS ----------------------------*/

QVariantList stringsToVariants( QStringList list )
{
  list.removeDuplicates();

  QVariantList variants;

  foreach( QString string, list )
  {
    /* If the string is empty, we wish to construct a NULL variant for
      entry into the database table. */
    if( string == "" )
    {
      variants << QVariant( QVariant::String );
    }
    else
    {
      variants << string;
    }
  }

  return variants;
}

/*--------------------------------- MEMBER FUNCTIONS ----------------------------------*/

GCBatchProcessorHelper::GCBatchProcessorHelper(const QDomDocument &domDoc) :
  m_unsorted(),
  m_records (),
  m_knownElements     (),
  m_knownAttributeKeys(),
  m_newElementsToAdd         (),
  m_newElementCommentsToAdd  (),
  m_newElementAttributesToAdd(),
  m_elementsToUpdate         (),
  m_elementCommentsToUpdate  (),
  m_elementAttributesToUpdate(),
  m_newAttributeKeysToAdd    (),
  m_newAttributeValuesToAdd  (),
  m_attributeKeysToUpdate    (),
  m_attributeValuesToUpdate  ()
{
  QDomElement root = domDoc.documentElement();
  createRecord( root );
  processElement( root );
  sortRecords();
}

/*--------------------------------------------------------------------------------------*/

void GCBatchProcessorHelper::processElement( const QDomElement &parentElement )
{
  QDomElement element = parentElement.firstChildElement();

  while ( !element.isNull() )
  {
    createRecord( element );
    processElement( element );
    element = element.nextSiblingElement();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCBatchProcessorHelper::createRecord( const QDomElement &element )
{
  GCElementRecord record;

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

  /* We also check if there are any comments associated with this element.  We'll
    operate on the assumption that an XML comment will appear directly before the element
    in question and also that such comments are siblings of the element in question. */
  if( element.previousSibling().nodeType() == QDomNode::CommentNode )
  {
    record.comments.append( element.previousSibling().toComment().nodeValue() );
  }

  /* We'll sort it all later, for now get the recursive calls out of the way. */
  m_unsorted.insert( element.tagName(), record );
}

/*--------------------------------------------------------------------------------------*/

void GCBatchProcessorHelper::sortRecords()
{
  foreach( QString element, m_unsorted.uniqueKeys() )
  {
    /* Retrieve all the records associated with this element. */
    QList< GCElementRecord > duplicateRecords = m_unsorted.values( element );

    /* If we have more than one record for the same element, then we need to consolidate
      all the attributes associated with this particular element. */
    if( duplicateRecords.size() > 1 )
    {
      GCElementRecord record;

      /* Just stick all the attributes and their values into a map for now
        so that we don't have to run the consolidation functionality on
        each iteration...that would just be silly... */
      QMultiMap< QString, QStringList > recordAttributes;

      for( int i = 0; i < duplicateRecords.size(); ++i )
      {
        /* Since we're already iterating through the list, we may as well consolidate the
          comments while we're at it (they are relatively simple to handle compared
          to the attribute value maps). */
        record.comments.append( duplicateRecords.at( i ).comments );
        recordAttributes += duplicateRecords.at( i ).attributes;
      }

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

      record.comments.removeDuplicates();
      m_records.insert( element, record );
    }
    else
    {
      m_records.insert( element, duplicateRecords.at( 0 ) );
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCBatchProcessorHelper::createVariantLists()
{
  /* QStringLists are used to simplify the process of removing duplicates from the lists.
    To ensure that we insert the correct values against the correct values against the
    correct keys in the record, we also populate the QStringLists with "" wherever a comment or
    attribute doesn't have any values (we wish to ensure that the indices are in sync
    at all times).  The "" values will be replaced with proper NULL values when inserting
    the new records (or updating the old ones) in the DB. */

  /* First see which of the records we created from the DOM doc are completely new
    and which ones we have prior knowledge of. */
  separateNewRecordsFromExisting();

  /* Deal with all the new elements first. */
  QStringList newElementCommentsToAdd;
  QStringList newElementAttributesToAdd;

  foreach( QVariant var, m_newElementsToAdd )
  {
    QString element = var.toString();

    if( !m_records.value( element ).comments.isEmpty() )
    {
      newElementCommentsToAdd << m_records.value( element ).comments.join( SEPARATOR );
    }
    else
    {
      newElementCommentsToAdd << "";
    }

    if( !m_records.value( element ).attributes.keys().isEmpty() )
    {
      QStringList attributeNames = m_records.value( element ).attributes.keys();
      newElementAttributesToAdd << attributeNames.join( SEPARATOR );
    }
    else
    {
      newElementAttributesToAdd << "";
    }
  }

  /* Now we deal with all the elements that will have to be updated. */
  QStringList elementCommentsToUpdate;
  QStringList elementAttributesToUpdate;

  foreach( QVariant var, m_elementsToUpdate )
  {
    QString element = var.toString();

    if( !m_records.value( element ).comments.isEmpty() )
    {
      elementCommentsToUpdate << m_records.value( element ).comments.join( SEPARATOR );
    }
    else
    {
      elementCommentsToUpdate << "";
    }

    if( !m_records.value( element ).attributes.keys().isEmpty() )
    {
      QStringList attributeNames = m_records.value( element ).attributes.keys();
      elementAttributesToUpdate << attributeNames.join( SEPARATOR );
    }
    else
    {
      elementAttributesToUpdate << "";
    }
  }

  /* Finally, we separate the new attribute keys and associated values
    from the existing ones. */
  QStringList newAttributeValuesToAdd;

  foreach( QVariant var, m_newAttributeKeysToAdd )
  {
    QString attributeKey = var.toString();
  }

  QStringList attributeValuesToUpdate;

  foreach( QVariant var, m_attributeKeysToUpdate )
  {
    QString attributeKey = var.toString();
  }
}

/*--------------------------------------------------------------------------------------*/

void GCBatchProcessorHelper::separateNewRecordsFromExisting()
{
  QList< QString > elementNames = m_records.keys();

  /* Separate new from existing elements. */
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

  /* Separate new from existing attribute keys. */
  foreach( QString element, elementNames )
  {
    QList< QString > attributeNames = m_records.value( element ).attributes.keys();

    foreach( QString attribute, attributeNames )
    {
      if( !m_knownAttributeKeys.contains( element + attribute ) )
      {
        m_newAttributeKeysToAdd << element + attribute;
      }
      else
      {
        m_attributeKeysToUpdate << element + attribute;
      }
    }
  }
}

/*--------------------------------------------------------------------------------------*/

void GCBatchProcessorHelper::setKnownElements( const QStringList &elementts )
{
  m_knownElements = elementts;
}

/*--------------------------------------------------------------------------------------*/

void GCBatchProcessorHelper::setKnownAttributes( const QStringList &attributeKeys )
{
  m_knownAttributeKeys = attributeKeys;
}

/*--------------------------------------------------------------------------------------*/

QVariantList GCBatchProcessorHelper::newElementsToAdd() const
{
  return m_newElementsToAdd;
}

/*--------------------------------------------------------------------------------------*/

QVariantList GCBatchProcessorHelper::newElementCommentsToAdd() const
{
  return m_newElementCommentsToAdd;
}

/*--------------------------------------------------------------------------------------*/

QVariantList GCBatchProcessorHelper::newElementAttributesToAdd() const
{
  return m_newElementAttributesToAdd;
}

/*--------------------------------------------------------------------------------------*/

QVariantList GCBatchProcessorHelper::elementsToUpdate() const
{
  return m_elementsToUpdate;
}

/*--------------------------------------------------------------------------------------*/

QVariantList GCBatchProcessorHelper::elementCommentsToUpdate() const
{
  return m_elementCommentsToUpdate;
}

/*--------------------------------------------------------------------------------------*/

QVariantList GCBatchProcessorHelper::elementAttributesToUpdate() const
{
  return m_elementAttributesToUpdate;
}

/*--------------------------------------------------------------------------------------*/

QVariantList GCBatchProcessorHelper::newAttributeKeysToAdd() const
{
  return m_newAttributeKeysToAdd;
}

/*--------------------------------------------------------------------------------------*/

QVariantList GCBatchProcessorHelper::newAttributeValuesToAdd() const
{
  return m_newAttributeValuesToAdd;
}

/*--------------------------------------------------------------------------------------*/

QVariantList GCBatchProcessorHelper::attributeKeysToUpdate() const
{
  return m_attributeKeysToUpdate;
}

/*--------------------------------------------------------------------------------------*/

QVariantList GCBatchProcessorHelper::attributeValuesToUpdate() const
{
  return m_attributeValuesToUpdate;
}

/*--------------------------------------------------------------------------------------*/
