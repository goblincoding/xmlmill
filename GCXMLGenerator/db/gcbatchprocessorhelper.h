/* Copyright (c) 2012 by William Hallatt.
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

#ifndef GCBATCHPROCESSORHELPER_H
#define GCBATCHPROCESSORHELPER_H

#include <QDomDocument>
#include <QMap>
#include <QMultiHash>
#include <QStringList>
#include <QVariantList>

/// Helper class assisting with batch updates to the database.

/**
  The purpose of this class is to (1) extract all the elements and their associated attributes
  and attribute values from the DOM document passed in as parameter to the constructor and
  (2) to consolidate the lot into QVariantLists that can be used as bind variables for prepared
  queries intended to be executed in batches (that's quite a mouthful, see "execBatch" in the Qt
  documentation for more information on this topic).

  The idea is not really to have a long-lived instance of this object in the calling object (i.e.
  it isn't intended to be used as a member variable, although it isn't prevented either), but rather
  to create a scoped local variable that should be created and set up as follows:
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
  /*! Constructor
      @param domDoc - the DOM document from which all information will be extracted.
      @param stringSeparator - the string sequence by which list elements in the database are
      separated (e.g. attribute values are stored as lists in the database, separated by
      a special character sequence.  In other words, although the database sees a list of
      attribute values as a single string, we can extract the list elements later if we
      know which string sequence was used in the creation of the string list).  This
      value should be unusual and unique.
      @param knownElements - the list of elements known to the active database, if empty,
      all the elements in the DOM will be assumed to be new.
      @param knownAttributes - the list of attributes known to the active database, if empty,
      all the attributes in the DOM will be assumed to be new.  */
  GCBatchProcessorHelper( const QDomDocument *domDoc,
                          const QString &stringSeparator,
                          const QStringList &knownElements,
                          const QStringList &knownAttributes );

  /*! Returns a list of all the new elements that should be added to the database. 
      \sa newElementChildrenToAdd
      \sa newElementAttributesToAdd */
  const QVariantList &newElementsToAdd() const;

  /*! Returns a list of lists of all new first level child elements that should be added to 
      the database. Each item in this list is a list of first level child elements corresponding
      to an element in the "new elements to add" list.  In other words, for each item in the "new
      elements to add" list, there is a corresponding QVariant item in this list (with the same
      index) that represents the list of the element's first level children.  Each QVariant consists of
      all these first level child elements concatenated into a single string value with the individuals
      separated by the unique string separator that was passed in as constructor parameter.
      Where an element does not have first level children, a NULL QVariant value is added to the
      list to ensure that the indices of all lists are kept in synch. 
      \sa newElementsToAdd
      \sa newElementAttributesToAdd */
  const QVariantList &newElementChildrenToAdd() const;

  /*! Returns a list of lists of all new associated attributes that should be added to 
      the database. Each item in this list is a list of associated attributes corresponding
      to an element in the "new elements to add" list.  In other words, for each item in the "new
      elements to add" list, there is a corresponding QVariant item in this list (with the same
      index) that represents the list of the element's associated attributes.  Each QVariant consists of
      all these associated attributes concatenated into a single string value with the individuals
      separated by the unique string separator that was passed in as constructor parameter.
      Where an element does not have associated attributes, a NULL QVariant value is added to the
      list to ensure that the indices of all lists are kept in synch. 
      \sa newElementsToAdd
      \sa newElementChildrenToAdd */
  const QVariantList &newElementAttributesToAdd() const;

  /*! Returns a list of all the elements that should be updated. 
      \sa elementChildrenToUpdate
      \sa elementAttributesToUpdate */
  const QVariantList &elementsToUpdate() const;

  /*! Returns a list of lists of all first level child elements corresponding to existing elements that 
      should be updated. Each item in this list is a list of first level child elements corresponding
      to an element in the "elements to update" list.  In other words, for each item in the "elements 
      to update" list, there is a corresponding QVariant item in this list (with the same
      index) that represents the list of the element's first level children.  Each QVariant consists of
      all these first level child elements concatenated into a single string value with the individuals
      separated by the unique string separator that was passed in as constructor parameter.
      Where an element does not have first level children, a NULL QVariant value is added to the
      list to ensure that the indices of all lists are kept in synch. 
      \sa elementsToUpdate
      \sa elementAttributesToUpdate */
  const QVariantList &elementChildrenToUpdate() const;

  /*! Returns a list of lists of all associated attributes corresponding to existing elements that 
      should be updated. Each item in this list is a list of associated attributes corresponding
      to an element in the "elements to update" list.  In other words, for each item in the "elements 
      to update" list, there is a corresponding QVariant item in this list (with the same
      index) that represents the list of the element's associated attributes.  Each QVariant consists of
      all these associated attributes concatenated into a single string value with the individuals
      separated by the unique string separator that was passed in as constructor parameter.
      Where an element does not have associated attributes, a NULL QVariant value is added to the
      list to ensure that the indices of all lists are kept in synch. 
      \sa elementsToUpdate
      \sa elementChildrenToUpdate */
  const QVariantList &elementAttributesToUpdate() const;

  /*! Returns a list of all the new attributes that should be added to the database. 
      \sa newAssociatedElementsToAdd()
      \sa newAttributeValuesToAdd() */
  const QVariantList &newAttributeKeysToAdd() const;

  /*! Returns a list of all the new associated elements that should be added to the database.
      All attributes are associated with specific elements (this allows us to save different values
      against attributes of the same name that are associated with different elements).
      Each item in this list is the specific element associated with an attribute in the "new attribute
      keys to add" list. 
      \sa newAttributeKeysToAdd()
      \sa newAttributeValuesToAdd() */
  const QVariantList &newAssociatedElementsToAdd() const;

  /*! Returns a list of lists of all new attribute values that should be added to 
      the database. Each item in this list is a list of known attribute values corresponding
      to an attribute in the "new attribute keys to add" list.  In other words, for each item in the 
      "new attribute keys to add" list, there is a corresponding QVariant item in this list (with the same
      index) that represents the list of the attribute's known values.  Each QVariant consists of
      all these known values concatenated into a single string value with the individuals
      separated by the unique string separator that was passed in as constructor parameter. 
      \sa newAttributeKeysToAdd()
      \sa newAssociatedElementsToAdd() */
  const QVariantList &newAttributeValuesToAdd() const;

  /*! Returns a list of all the attribute keys that should be updated.
      \sa associatedElementsToUpdate()
      \sa attributeValuesToUpdate() */
  const QVariantList &attributeKeysToUpdate() const;

  /*! Returns a list of all the associated elements corresponding to existing attribute keys
      that should be updated. All attributes are associated with specific elements (this allows us to 
      save different values against attributes of the same name that are associated with different elements).
      Each item in this list is the specific element associated with an attribute in the "attribute keys to
      update" list. 
      \sa attributeKeysToUpdate() 
      \sa attributeValuesToUpdate() */
  const QVariantList &associatedElementsToUpdate() const;

  /*! Returns a list of lists of all attribute values associated with existing attributes that should be 
      updated. Each item in this list is a list of known attribute values corresponding
      to an attribute in the "attribute keys to update" list.  In other words, for each item in the 
      "attribute keys to update" list, there is a corresponding QVariant item in this list (with the same
      index) that represents the list of the attribute's known values.  Each QVariant consists of
      all these known values concatenated into a single string value with the individuals
      separated by the unique string separator that was passed in as constructor parameter. 
      \sa attributeKeysToUpdate() 
      \sa associatedElementsToUpdate() */
  const QVariantList &attributeValuesToUpdate() const;

private:
  /*! Processes an element by extracting information related to its first level children, associated
      attributes and the values of these attributes. This function is called recursively in order to traverse
      the DOM hierarchy.  */
  void processElement( const QDomElement &parentElement );

  /*! Creates an "ElementRecord" from "element".  Called from within processElement, this function
      creates the records and adds them to the record map without checking for duplicates.
      \sa sortRecords */
  void createRecord( const QDomElement &element );

  /*! Sorts all the element records in the unsorted record map and consolidates values where duplicates
      are encountered.
      \sa createRecord */
  void sortRecords();

  /*! Creates the lists of QVariants representing elements, attributes and values. */
  void createVariantLists();

  QString m_stringListSeparator;

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

  /*! Represents a single element's associated first level children,
      attributes and known attribute values. */
  struct ElementRecord
  {
    QStringList children;
    QMap< QString /*name*/, QStringList /*values*/ > attributes;

    ElementRecord()
      :
        children  (),
        attributes()
    {}
  };

  QMultiHash< QString /*element*/, ElementRecord > m_unsorted;
  QMap      < QString /*element*/, ElementRecord > m_records;
};

#endif // GCBATCHPROCESSORHELPER_H
