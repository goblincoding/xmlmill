/* Copyright (c) 2012 - 2013 by William Hallatt.
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
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#include "addsnippetsform.h"
#include "ui_addsnippetsform.h"
#include "db/dbinterface.h"
#include "utils/combobox.h"
#include "utils/messagespace.h"
#include "utils/globalspace.h"
#include "utils/treewidgetitem.h"

#include <QCheckBox>
#include <QMessageBox>

/*----------------------------------------------------------------------------*/

const int LABELCOLUMN = 0;
const int COMBOCOLUMN = 1;
const int INCRCOLUMN = 2;

/*----------------------------------------------------------------------------*/

AddSnippetsForm::AddSnippetsForm(const QString &elementName,
                                 TreeWidgetItem *parentItem, QWidget *parent)
    : QDialog(parent), ui(new Ui::AddSnippetsForm), m_parentItem(parentItem),
      m_treeItemActivated(false) {
  ui->setupUi(this);
  ui->tableWidget->setFont(QFont(GlobalSpace::FONT, GlobalSpace::FONTSIZE));
  ui->tableWidget->horizontalHeader()->setFont(
      QFont(GlobalSpace::FONT, GlobalSpace::FONTSIZE));

  ui->tableWidget->setColumnWidth(INCRCOLUMN, 40); // restricted for checkbox
  ui->treeWidget->setColumnWidth(0, 50);           // restricted for checkbox
  ui->showHelpButton->setVisible(GlobalSpace::showHelpButtons());

  ui->treeWidget->populateFromDatabase(elementName);
  ui->treeWidget->setAllCheckStates(Qt::Checked);
  elementSelected(ui->treeWidget->CurrentItem(), 0);

  connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));
  connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addSnippet()));
  connect(ui->showHelpButton, SIGNAL(clicked()), this, SLOT(showHelp()));
  connect(ui->tableWidget, SIGNAL(itemChanged(QTableWidgetItem *)), this,
          SLOT(attributeChanged(QTableWidgetItem *)));
  connect(ui->treeWidget, SIGNAL(CurrentItemSelected(TreeWidgetItem *, int)),
          this, SLOT(elementSelected(TreeWidgetItem *, int)));

  setAttribute(Qt::WA_DeleteOnClose);
}

/*----------------------------------------------------------------------------*/

AddSnippetsForm::~AddSnippetsForm() { delete ui; }

/*----------------------------------------------------------------------------*/

void AddSnippetsForm::elementSelected(TreeWidgetItem *item, int column) {
  Q_UNUSED(column);

  if (item) {
    m_treeItemActivated = true;

    ui->tableWidget->clearContents(); // also deletes current items
    ui->tableWidget->setRowCount(0);

    /* Populate the table widget with the attributes and values associated with
     * the element selected. */
    QString elementName = item->name();
    QStringList attributeNames =
        DataBaseInterface::instance()->attributes(elementName);

    /* Create and add the "increment" checkbox to the first column of the table
     * widget, add all the known attribute names to the cells in the second
     * column of the table widget, create and populate combo boxes with the
     * values associated with the attributes in question and insert the combo
     * boxes into the third column of the table widget. */
    for (int i = 0; i < attributeNames.count(); ++i) {
      ui->tableWidget->setRowCount(i + 1);

      QCheckBox *checkBox = new QCheckBox;

      /* Overrides main style sheet. */
      checkBox->setStyleSheet("QCheckBox{ padding-right: 1px; }"
                              "QCheckBox::indicator{ subcontrol-position: "
                              "center; width: 15px; height: 15px; }");

      ui->tableWidget->setCellWidget(i, INCRCOLUMN, checkBox);
      connect(checkBox, SIGNAL(clicked()), this, SLOT(attributeValueChanged()));

      QDomAttr attribute =
          item->element().attributeNode(attributeNames.at(i)).toAttr();
      checkBox->setChecked(item->incrementAttribute(attribute.name()));

      /* Items are editable by default, disable this option. */
      QTableWidgetItem *label = new QTableWidgetItem(attributeNames.at(i));
      label->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable |
                      Qt::ItemIsUserCheckable);
      ui->tableWidget->setItem(i, LABELCOLUMN, label);

      ComboBox *attributeCombo = new ComboBox;
      attributeCombo->addItems(DataBaseInterface::instance()->attributeValues(
          elementName, attributeNames.at(i)));
      attributeCombo->setEditable(true);
      attributeCombo->setCurrentIndex(attributeCombo->findText(
          item->element().attribute(attributeNames.at(i))));

      connect(attributeCombo, SIGNAL(currentIndexChanged(QString)), this,
              SLOT(attributeValueChanged()));

      if (item->attributeIncluded(attributeNames.at(i))) {
        label->setCheckState(Qt::Checked);
        attributeCombo->setEnabled(true);
      } else {
        label->setCheckState(Qt::Unchecked);
        attributeCombo->setEnabled(false);
      }

      ui->tableWidget->setCellWidget(i, COMBOCOLUMN, attributeCombo);

      if (item->checkState(0) == Qt::Unchecked) {
        ui->tableWidget->setEnabled(false);
      } else {
        ui->tableWidget->setEnabled(true);
      }
    }

    updateCheckStates(item);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(
        LABELCOLUMN, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(
        COMBOCOLUMN, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(
        INCRCOLUMN, QHeaderView::Fixed);

    m_treeItemActivated = false;
  }
}

/*----------------------------------------------------------------------------*/

void AddSnippetsForm::attributeChanged(QTableWidgetItem *item) const {
  if (!m_treeItemActivated) {
    TreeWidgetItem *treeItem = ui->treeWidget->CurrentItem();
    ComboBox *attributeValueCombo = dynamic_cast<ComboBox *>(
        ui->tableWidget->cellWidget(item->row(), COMBOCOLUMN));
    QCheckBox *checkBox = dynamic_cast<QCheckBox *>(
        ui->tableWidget->cellWidget(item->row(), INCRCOLUMN));

    if (item->checkState() == Qt::Checked) {
      attributeValueCombo->setEnabled(true);
      checkBox->setEnabled(true);
      treeItem->includeAttribute(item->text(),
                                 attributeValueCombo->currentText());
    } else {
      attributeValueCombo->setEnabled(false);
      checkBox->setEnabled(false);
      treeItem->excludeAttribute(item->text());
    }
  }
}

/*----------------------------------------------------------------------------*/

void AddSnippetsForm::attributeValueChanged() const {
  /* Update the element's attribute inclusions, values and value increment
   * flags. */
  TreeWidgetItem *treeItem = ui->treeWidget->CurrentItem();
  QDomNamedNodeMap attributes = treeItem->element().attributes();

  /* The table doesn't know which attributes are included or excluded and
   * contains rows corresponding to all the attributes associated with the
   * element. We only wish to act on attributes currently included. */
  for (int i = 0; i < attributes.size(); ++i) {
    for (int j = 0; j < ui->tableWidget->rowCount(); ++j) {
      QString attributeName = ui->tableWidget->item(j, LABELCOLUMN)->text();

      if (attributeName == attributes.item(i).nodeName()) {
        QCheckBox *checkBox = dynamic_cast<QCheckBox *>(
            ui->tableWidget->cellWidget(j, INCRCOLUMN));
        ComboBox *comboBox = dynamic_cast<ComboBox *>(
            ui->tableWidget->cellWidget(j, COMBOCOLUMN));
        QString attributeValue = comboBox->currentText();

        if (treeItem->attributeIncluded(attributeName)) {
          treeItem->includeAttribute(attributeName, attributeValue);
        }

        treeItem->setIncrementAttribute(attributeName, checkBox->isChecked());
      }
    }
  }
}

/*----------------------------------------------------------------------------*/

void AddSnippetsForm::addSnippet() {
  QList<TreeWidgetItem *> includedItems =
      ui->treeWidget->includedTreeWidgetItems();

  /* Add the required number of snippets. */
  for (int i = 0; i < ui->spinBox->value(); ++i) {
    /* Update all the included elements and attribute values. */
    for (int j = 0; j < includedItems.size(); ++j) {
      TreeWidgetItem *localItem = includedItems.at(j);

      /* Sets a "restore point" so that we may increment attribute values and
       * return to the previously fixed values (to avoid incrementing an
       * incremented value). */
      localItem->fixAttributeValues();

      QString elementName = localItem->element().tagName();
      QDomNamedNodeMap attributes = localItem->element().attributes();

      for (int k = 0; k < attributes.size(); ++k) {
        QDomAttr attr = attributes.item(k).toAttr();
        QString attributeValue = localItem->fixedValue(attr.name());

        if (localItem->incrementAttribute(attr.name())) {
          /* Check if this is a number (if it contains any non-digit character).
           */
          if (!attributeValue.contains(QRegExp("\\D+"))) {
            bool ok(false);
            int intValue = attributeValue.toInt(&ok);

            if (ok) {
              intValue += i;
              attributeValue = QString("%1").arg(intValue);
            }
          } else {
            /* If the value contains some string characters, it's a string value
             * and that's all there is to it (it's not our responsibility to
             * check that someone isn't incrementing "false", e.g.). */
            attributeValue += QString("%1").arg(i);
          }

          localItem->element().setAttribute(attr.name(), attributeValue);
        }

        /* This call does nothing if the attribute value already exists. */
        DataBaseInterface::instance()->updateAttributeValues(
            elementName, attr.name(), QStringList(attributeValue));
      }
    }

    emit snippetAdded(m_parentItem,
                      ui->treeWidget->cloneDocument().toElement());

    /* Restore values. */
    for (int j = 0; j < includedItems.size(); ++j) {
      includedItems.at(j)->revertToFixedValues();
    }
  }
}

/*----------------------------------------------------------------------------*/

void AddSnippetsForm::updateCheckStates(TreeWidgetItem *item) const {
  /* Checking or unchecking an item must recursively update its children as
   * well. */
  if (item->checkState(0) == Qt::Checked) {
    item->setExcludeElement(false);

    /* When a low-level child is activated, we need to also update its parent
     * tree all the way up to the root element since including a child
     * automatically implies that the parent element is included. */
    TreeWidgetItem *parent = item->Parent();

    while (parent && (parent->checkState(0) != Qt::Checked)) {
      parent->setExcludeElement(false);
      parent->setCheckState(0, Qt::Checked);
      parent = parent->Parent();
    }
  } else {
    item->setExcludeElement(true);

    for (int i = 0; i < item->childCount(); ++i) {
      item->child(i)->setCheckState(0, item->checkState(0));
      updateCheckStates(item->Child(i));
    }
  }
}

/*----------------------------------------------------------------------------*/

void AddSnippetsForm::showHelp() {
  QMessageBox::information(
      this, "How this works...",
      "Use this form to create XML snippets with the default values "
      "you specify. \n\n"
      "Unchecking an element will exclude it (and all of its children) from "
      "the snippet (similarly, check attributes that you want to be included "
      "in "
      "your snippet(s)). \n\n"
      "If you tick the \"Incr\" (increment) option next to an attribute value, "
      "then the "
      "value you provided will be incremented with \"1\" for however many "
      "snippets you generate. \n\n"
      "For example, if you specify \"10\" as an attribute value, then the "
      "first snippet will assign \"10\" to the attribute in question, "
      "the second will have \"11\", the third, \"12\", etc. \n\n"
      "Strings will have the incremented value appended to the name (\"true\" "
      "and \"false\" values are treated as strings, so be careful).");
}

/*----------------------------------------------------------------------------*/
