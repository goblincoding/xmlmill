#ifndef DOMDELEGATE_H
#define DOMDELEGATE_H

#include <QStyledItemDelegate>

//----------------------------------------------------------------------

class DomDelegate : public QStyledItemDelegate {
  Q_OBJECT
public:
  explicit DomDelegate(QObject *parent = 0);

  // QAbstractItemDelegate interface
public:
  virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
                                const QModelIndex &) const;
  virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
  virtual void setModelData(QWidget *editor, QAbstractItemModel *model,
                            const QModelIndex &index) const;
  virtual void updateEditorGeometry(QWidget *editor,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &) const;
};

#endif // DOMDELEGATE_H
