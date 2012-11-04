#ifndef GCBATCHPROCESSORHELPER_H
#define GCBATCHPROCESSORHELPER_H

#include <QDomDocument>
#include <QMap>
#include <QMultiHash>
#include <QStringList>
#include <QVariantList>

/*--------------------------------------------------------------------------------------*/

class GCBatchProcessorHelper
{
public:
  GCBatchProcessorHelper ( const QDomDocument &domDoc );
  void setKnownElements  ( const QStringList &elementts );
  void setKnownAttributes( const QStringList &attributeKeys );
  void createVariantLists();

  QVariantList newElementsToAdd() const;
  QVariantList newElementCommentsToAdd() const;
  QVariantList newElementAttributesToAdd() const;

  QVariantList elementsToUpdate() const;
  QVariantList elementCommentsToUpdate() const;
  QVariantList elementAttributesToUpdate() const;

  QVariantList newAttributeKeysToAdd() const;
  QVariantList newAttributeValuesToAdd() const;

  QVariantList attributeKeysToUpdate() const;
  QVariantList attributeValuesToUpdate() const;

private:
  void processElement( const QDomElement &parentElement );
  void createRecord  ( const QDomElement &element );
  void sortRecords();

  QStringList  m_knownElements;
  QStringList  m_knownAttributeKeys;

  QVariantList m_newElementsToAdd;
  QVariantList m_newElementCommentsToAdd;
  QVariantList m_newElementAttributesToAdd;

  QVariantList m_elementsToUpdate;
  QVariantList m_elementCommentsToUpdate;
  QVariantList m_elementAttributesToUpdate;

  QVariantList m_newAttributeKeysToAdd;
  QVariantList m_newAttributeValuesToAdd;

  QVariantList m_attributeKeysToUpdate;
  QVariantList m_attributeValuesToUpdate;

  /* Represents a single element and its associated comments and attributes. */
  struct GCElementRecord
  {
    QStringList comments;
    QMap< QString /*name*/, QStringList /*values*/ > attributes;

    GCElementRecord()
      :
        comments  (),
        attributes()
    {}
  };

  QMultiHash< QString /*element*/, GCElementRecord > m_unsorted;
  QMap      < QString /*element*/, GCElementRecord > m_records;
};

#endif // GCBATCHPROCESSORHELPER_H
