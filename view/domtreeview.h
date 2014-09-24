#ifndef DOMTREEVIEW_H
#define DOMTREEVIEW_H

#include <QTreeView>

class DomTreeView : public QTreeView
{
  Q_OBJECT
public:
  explicit DomTreeView(QWidget *parent = 0);

signals:

public slots:

private slots:
  /*! Connected to a context menu action.  Renames the element on which the
   * context menu action was invoked. An element with the new name will be added
   * to the DB (if it doesn't yet exist) with the same associated attributes and
   * attribute values as the element name it is replacing (the "old" element
   * will not be removed from the DB). All occurrences of the old name
   * throughout the current DOM will be replaced with the new name and the tree
   * widget will be updated accordingly. */
  void renameItem();

  /*! Connected to a context menu action. Removes the item (and it's
   * corresponding element) on which the context menu action was invoked from
   * the tree and underlying DOM. This function will furthermore remove all
   * comment nodes directly above the element node. */
  void removeItem();

  /*! Connected to a context menu action.  Moves the active (selected) item to
     the level of its parent.
      \sa stepDown */
  void stepUp();

  /*! Connected to a context menu action.  Moves the active (selected) item to
     the level of its children.
      \sa stepUp */
  void stepDown();

  /*! Connected to a context menu action.  Expands active (selected) item.
      \sa collapse */
  void expandSelection();

  /*! Connected to a context menu action.  Collapses the active (selected) item.
      \sa expand */
  void collapseSelection();


};

#endif // DOMTREEVIEW_H
