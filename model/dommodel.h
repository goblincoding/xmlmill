#ifndef DOMMODEL_H
#define DOMMODEL_H

#include <QAbstractItemModel>
#include <QDomDocument>
#include <QModelIndex>

//----------------------------------------------------------------------

class DomItem;

//----------------------------------------------------------------------

class DomModel : public QAbstractItemModel {
  Q_OBJECT

public:
  explicit DomModel(QDomDocument document, QObject *parent = 0);
  virtual ~DomModel();

  // QAbstractItemModel interface
public:
  virtual QVariant headerData(int section, Qt::Orientation orientation,
                              int role = Qt::DisplayRole) const;

  virtual QVariant data(const QModelIndex &index, int role) const;

  virtual bool setData(const QModelIndex &index, const QVariant &value,
                       int role);

  virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  virtual QModelIndex index(int row, int column,
                            const QModelIndex &parent = QModelIndex()) const;

  virtual QModelIndex parent(const QModelIndex &child) const;

  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
  DomItem *domItem(const QModelIndex &index);

private:
  QDomDocument domDocument;
  DomItem *rootItem;
};

#endif // DOMMODEL_H
