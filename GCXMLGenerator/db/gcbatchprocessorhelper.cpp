#include "gcbatchprocessorhelper.h"

/*--------------------------------------------------------------------------------------*/

GCBatchProcessorHelper::GCBatchProcessorHelper(const QDomDocument &domDoc) :
  m_unsorted(),
  m_records ()
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

  /* We'll sort it all later. */
  m_unsorted.insert( element.tagName(), record );
}

/*--------------------------------------------------------------------------------------*/

void GCBatchProcessorHelper::sortRecords()
{
  foreach( QString element, m_unsorted.uniqueKeys() )
  {
    /* Retrieve all the records associated with this element. */
    QList< GCElementRecord > duplicateRecords = m_unsorted.values( element );

    /* If we have more than one record for the same element, consolidate. */
    if( duplicateRecords.size() > 1 )
    {
      GCElementRecord record;

      /* Just stick all the attributes and their values into a map (it's the quickest). */
      QMultiMap< QString, QStringList > recordAttributes;

      for( int i = 0; i < duplicateRecords.size(); ++i )
      {
        record.comments.append( duplicateRecords.at( i ).comments );
        recordAttributes += QMultiMap( duplicateRecords.at( i ).attributes );
      }

      /* Now we can sort out the chaos. */
      foreach( QString attribute, recordAttributes.uniqueKeys() )
      {
        QStringList finalListOfAttributeValues;

        /* Run through the list of lists and make one daddy QStringList. */
        QList< QStringList > attributeValues = recordAttributes.values( attribute );

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

const QMap< QString, GCElementRecord > &GCBatchProcessorHelper::getRecords() const
{
  return m_records;
}

/*--------------------------------------------------------------------------------------*/
