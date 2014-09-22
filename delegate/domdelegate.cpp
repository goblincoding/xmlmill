#include "domdelegate.h"

#include <QPlainTextEdit>

//----------------------------------------------------------------------

DomDelegate::DomDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

//----------------------------------------------------------------------

QWidget *DomDelegate::createEditor(QWidget *parent,
                                   const QStyleOptionViewItem & /*option*/,
                                   const QModelIndex & /*index*/) const {
  QPlainTextEdit *textEdit = new QPlainTextEdit(parent);
  return textEdit;
}

//----------------------------------------------------------------------

void DomDelegate::setEditorData(QWidget *editor,
                                const QModelIndex &index) const {
  QString value = index.model()->data(index, Qt::DisplayRole).toString();
  QPlainTextEdit *textEdit = dynamic_cast<QPlainTextEdit *>(editor);

  if (textEdit) {
    textEdit->setPlainText(value);
  }
}

//----------------------------------------------------------------------

void DomDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                               const QModelIndex &index) const {
  QPlainTextEdit *textEdit = dynamic_cast<QPlainTextEdit *>(editor);

  if (textEdit) {
    QString value = textEdit->toPlainText();
    model->setData(index, value, Qt::DisplayRole);
  }
}

//----------------------------------------------------------------------

void DomDelegate::updateEditorGeometry(QWidget *editor,
                                       const QStyleOptionViewItem &option,
                                       const QModelIndex & /*index*/) const {
  editor->setGeometry(option.rect);
}

//----------------------------------------------------------------------
