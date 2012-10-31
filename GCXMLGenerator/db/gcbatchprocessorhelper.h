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
  const QMap< QString, GCElementRecord > &getRecords() const;

private:
  void processElement( const QDomElement &parentElement );
  void createRecord  ( const QDomElement &element );
  void sortRecords();

  QMultiHash< QString /*element*/, GCElementRecord > m_unsorted;
  QMap      < QString /*element*/, GCElementRecord > m_records;
};

#endif // GCBATCHPROCESSORHELPER_H
