#ifndef GCBATCHPROCESSORHELPER_H
#define GCBATCHPROCESSORHELPER_H

#include <QDomDocument>
#include <QMap>
#include <QMultiHash>
#include <QStringList>
#include <QVariantList>

/*--------------------------------------------------------------------------------------*/
/*
  The purpose of this class is to (1) extract all the elements and their associated attributes
  and attribute values from the DOM document passed in as parameter to the constructor and
  (2) to consolidate the lot into QVariantLists that can be used as bind variables for prepared
  queries intended to be executed in batches (see "execBatch" in the Qt documentation for
  more information on this topic). The idea is not really to have a long-lived instance of
  this object in the calling object (i.e. it isn't intended to be used as a member variable,
  although it isn't prevented either), but rather to create a scoped local variable that should be
  created and set up as follows:
    * Create an instance.
    * Call setKnownElements and setKnownAttributes so that this object may know which
       elements and attribute names are already known to the database (if these two
       lists aren't set, everything from within the DOM will be considered new).
    * Call createVariantLists() (this is compulsory to populate the variant lists)
    * Call the getters to retrieve the bind variable lists.

  This class has also been specifically designed to be used in conjunction with GCDatabaseInterface.
*/

class GCBatchProcessorHelper
{
public:
  GCBatchProcessorHelper ( const QDomDocument *domDoc );
  void setKnownElements  ( const QStringList &elements );
  void setKnownAttributes( const QStringList &attributeKeys );

  /* This function must be called before calling the getters. Failing
    to do so will result in either empty variant lists or variant
    lists containing incorrect/invalid values being returned. */
  void createVariantLists();

  QVariantList newElementsToAdd() const;
  QVariantList newElementChildrenToAdd() const;   // first level children only
  QVariantList newElementAttributesToAdd() const;

  QVariantList elementsToUpdate() const;
  QVariantList elementChildrenToUpdate() const;   // first level children only
  QVariantList elementAttributesToUpdate() const;

  QVariantList newAttributeKeysToAdd() const;
  QVariantList newAssociatedElementsToAdd() const;
  QVariantList newAttributeValuesToAdd() const;

  QVariantList attributeKeysToUpdate() const;
  QVariantList associatedElementsToUpdate() const;
  QVariantList attributeValuesToUpdate() const;

private:
  void processElement( const QDomElement &parentElement );
  void createRecord  ( const QDomElement &element );
  void sortRecords();

  QStringList  m_knownElements;
  QStringList  m_knownAttributeKeys;

  QVariantList m_newElementsToAdd;
  QVariantList m_newElementChildrenToAdd;
  QVariantList m_newElementAttributesToAdd;

  QVariantList m_elementsToUpdate;
  QVariantList m_elementChildrenToUpdate;
  QVariantList m_elementAttributesToUpdate;

  QVariantList m_newAttributeKeysToAdd;
  QVariantList m_newAssociatedElementsToAdd;
  QVariantList m_newAttributeValuesToAdd;

  QVariantList m_attributeKeysToUpdate;
  QVariantList m_associatedElementsToUpdate;
  QVariantList m_attributeValuesToUpdate;

  /* Represents a single element's associated first level children,
    attributes and known attribute values. */
  struct GCElementRecord
  {
    QStringList children;
    QMap< QString /*name*/, QStringList /*values*/ > attributes;

    GCElementRecord()
      :
        children  (),
        attributes()
    {}
  };

  QMultiHash< QString /*element*/, GCElementRecord > m_unsorted;
  QMap      < QString /*element*/, GCElementRecord > m_records;
};

#endif // GCBATCHPROCESSORHELPER_H
