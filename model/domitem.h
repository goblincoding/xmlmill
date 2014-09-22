#ifndef DOMITEM_H
#define DOMITEM_H

#include <QDomNode>
#include <QHash>
#include <QVariant>

//----------------------------------------------------------------------

class DomItem {
public:
  DomItem(QDomNode &node, int row, DomItem *parent = 0);
  ~DomItem();

  QVariant data(const QModelIndex &index, int role) const;
  bool setData(const QModelIndex &index, const QVariant &value);

  DomItem *child(int i);
  DomItem *parent();
  QDomNode node() const;
  int row();

private:
  QString toString() const;
  QString elementString() const;
  QString commentString() const;

private:
  QDomNode m_domNode;
  int m_rowNumber;

  DomItem *m_parentItem;
  QHash<int, DomItem *> m_childItems;
};

#endif // DOMITEM_H
