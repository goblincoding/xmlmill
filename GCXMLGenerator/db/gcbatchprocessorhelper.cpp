#include "gcbatchprocessorhelper.h"
#include "utils/gcglobals.h"

/*--------------------------------- MEMBER FUNCTIONS ----------------------------------*/

GCBatchProcessorHelper::GCBatchProcessorHelper(const QDomDocument &domDoc) :
  m_knownElements            (),
  m_knownAttributeKeys       (),
  m_newElementsToAdd         (),
  m_newElementCommentsToAdd  (),
  m_newElementAttributesToAdd(),
  m_elementsToUpdate         (),
  m_elementCommentsToUpdate  (),
  m_elementAttributesToUpdate(),
  m_newAttributeKeysToAdd    (),
  m_newAttributeValuesToAdd  (),
  m_attributeKeysToUpdate    (),
  m_attributeValuesToUpdate  (),
  m_unsorted                 (),
  m_records                  ()
{
  QDomElement root = domDoc.documentElement();
  createRecord( root );
  processElement( root );   // kicks off a chain of recursive DOM element traversals
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

    if( !m_records.value( element ).comments.isEmpty() )
    {
      m_newElementCommentsToAdd << m_records.value( element ).comments.join( SEPARATOR );
    }
    else
    {
      m_newElementCommentsToAdd << QVariant( QVariant::String );
    }

    if( !m_records.value( element ).attributes.keys().isEmpty() )
    {
      QStringList attributeNames = m_records.value( element ).attributes.keys();
      m_newElementAttributesToAdd << attributeNames.join( SEPARATOR );
    }
    else
    {
      m_newElementAttributesToAdd << QVariant( QVariant::String );
    }
  }

  /* Now we deal with all the elements that will have to be updated. */
  foreach( QVariant var, m_elementsToUpdate )
  {
    QString element = var.toString();

    if( !m_records.value( element ).comments.isEmpty() )
    {
      m_elementCommentsToUpdate << m_records.value( element ).comments.join( SEPARATOR );
    }
    else
    {
      m_elementCommentsToUpdate << QVariant( QVariant::String );
    }

    if( !m_records.value( element ).attributes.keys().isEmpty() )
    {
      QStringList attributeNames = m_records.value( element ).attributes.keys();
      m_elementAttributesToUpdate << attributeNames.join( SEPARATOR );
    }
    else
    {
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

      if( !m_knownAttributeKeys.contains( element + attribute ) )
      {
        m_newAttributeKeysToAdd << element + attribute;

        if( !attributeValues.isEmpty() )
        {
          m_newAttributeValuesToAdd << attributeValues.join( SEPARATOR );
        }
        else
        {
          m_newAttributeValuesToAdd << QVariant( QVariant::String );
        }
      }
      else
      {
        m_attributeKeysToUpdate << element + attribute;

        if( !attributeValues.isEmpty() )
        {
          m_attributeValuesToUpdate << attributeValues.join( SEPARATOR );
        }
        else
        {
          m_attributeValuesToUpdate << QVariant( QVariant::String );
        }
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
