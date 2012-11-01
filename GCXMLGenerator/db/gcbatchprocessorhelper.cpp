#include "gcbatchprocessorhelper.h"
#include "utils/gcglobals.h"

/*--------------------------------------------------------------------------------------*/

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
  m_attributeKeysToAdd       (),
  m_attributeValuesToAdd     (),
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
//  QStringList  helperElementNames = helper.getElementNames();

//  QStringList  existingElements = knownElements();
//  QVariantList elementsToUpdate;
//  QVariantList elementsToAdd;

//  /* Separate new from existing elements. */
//  for( int i = 0; i < helperElementNames.size(); ++i )
//  {
//    if( existingElements.contains( helperElementNames.at( i ) ) )
//    {
//      elementsToUpdate << helperElementNames.at( i );
//    }
//    else
//    {
//      elementsToAdd << helperElementNames.at( i );
//    }
//  }

//  /* TODO: I haven't figured out a way to batch process UPDATES...the problem is that we will have to
//    extract what is already there in order to add what is new and not overwrite what exists.
//    The only way forward may be to go the GCElementRecord route... */
//  QVariantList commentsToAdd;               // goes into xmlelements table
//  QVariantList attributesToAdd;             // goes into xmlelements table
//  QVariantList elementAttributesToAdd;      // goes into xmlattributevalues table
//  QVariantList elementAttributeValuesToAdd; // goes into xmlattributevalues table

//  foreach( QVariant elementVariant, elementsToAdd )
//  {
//    QString element = elementVariant.toString();
//    QString comments = helper.getElementComments( element ).join( SEPARATOR );

//    if( !comments.isEmpty() )
//    {
//      commentsToAdd << comments;
//    }
//    else
//    {
//      commentsToAdd << QVariant( QVariant::String );
//    }

//    QString attributes = helper.getAttributeNames( element ).join( SEPARATOR );

//    if( !attributes.isEmpty() )
//    {
//      attributesToAdd << attributes;

//      foreach( QString attribute, attributes )
//      {
//        elementAttributesToAdd << element + attribute;

//        QString elementAttributeValues = helper.getAttributeValues( element, attribute ).join( SEPARATOR );

//        if( !elementAttributeValues.isEmpty() )
//        {
//          elementAttributeValuesToAdd << elementAttributeValues;
//        }
//        else
//        {
//          elementAttributeValuesToAdd << QVariant( QVariant::String );
//        }
//      }
//    }
//    else
//    {
//      attributesToAdd << QVariant( QVariant::String );
//    }
//  }
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

QVariantList GCBatchProcessorHelper::attributeKeysToAdd() const
{
  return m_attributeKeysToAdd;
}

/*--------------------------------------------------------------------------------------*/

QVariantList GCBatchProcessorHelper::attributeValuesToAdd() const
{
  return m_attributeValuesToAdd;
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
