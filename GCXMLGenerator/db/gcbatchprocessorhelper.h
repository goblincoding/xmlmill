#ifndef GCBATCHPROCESSORHELPER_H
#define GCBATCHPROCESSORHELPER_H

#include <QDomDocument>
#include <QMap>
#include <QMultiHash>
#include <QStringList>

/*--------------------------------------------------------------------------------------*/

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

/*--------------------------------------------------------------------------------------*/

class GCBatchProcessorHelper
{
public:
  GCBatchProcessorHelper( const QDomDocument &domDoc );
  QStringList getElementNames() const;
  QStringList getElementComments( const QString &element ) const;
  QStringList getAttributeNames ( const QString &element ) const;
  QStringList getAttributeValues( const QString &element, const QString &attribute ) const;

private:
  void processElement( const QDomElement &parentElement );
  void createRecord  ( const QDomElement &element );
  void sortRecords();

  QMultiHash< QString /*element*/, GCElementRecord > m_unsorted;
  QMap      < QString /*element*/, GCElementRecord > m_records;
};

#endif // GCBATCHPROCESSORHELPER_H
