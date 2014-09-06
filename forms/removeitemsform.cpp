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

#include "removeitemsform.h"
#include "ui_removeitemsform.h"
#include "db/dbinterface.h"
#include "utils/messagespace.h"
#include "utils/globalsettings.h"
#include "utils/treewidgetitem.h"

#include <QMessageBox>

/*----------------------------------------------------------------------------*/

RemoveItemsForm::RemoveItemsForm(QWidget *parent)
    : QDialog(parent), ui(new Ui::RemoveItemsForm), m_currentElement(""),
      m_currentElementParent(""), m_currentAttribute(""), m_deletedElements() {
  ui->setupUi(this);
  ui->showAttributeHelpButton->setVisible(GlobalSettings::showHelpButtons());
  ui->showElementHelpButton->setVisible(GlobalSettings::showHelpButtons());

  connect(ui->showElementHelpButton, SIGNAL(clicked()), this,
          SLOT(showElementHelp()));
  connect(ui->showAttributeHelpButton, SIGNAL(clicked()), this,
          SLOT(showAttributeHelp()));
  connect(ui->updateValuesButton, SIGNAL(clicked()), this,
          SLOT(updateAttributeValues()));
  connect(ui->deleteAttributeButton, SIGNAL(clicked()), this,
          SLOT(deleteAttribute()));
  connect(ui->deleteElementButton, SIGNAL(clicked()), this,
          SLOT(deleteElement()));
  connect(ui->removeFromParentButton, SIGNAL(clicked()), this,
          SLOT(removeChildElement()));

  connect(ui->treeWidget, SIGNAL(CurrentItemSelected(TreeWidgetItem *, int)),
          this, SLOT(elementSelected(TreeWidgetItem *, int)));
  connect(ui->comboBox, SIGNAL(currentIndexChanged(QString)), this,
          SLOT(attributeActivated(QString)));

  ui->treeWidget->populateFromDatabase();

  setAttribute(Qt::WA_DeleteOnClose);
}

/*----------------------------------------------------------------------------*/

RemoveItemsForm::~RemoveItemsForm() { delete ui; }

/*----------------------------------------------------------------------------*/

void RemoveItemsForm::elementSelected(TreeWidgetItem *item, int column) {
  Q_UNUSED(column);

  if (item) {
    if (item->Parent()) {
      m_currentElementParent = item->Parent()->name();
    }

    m_currentElement = item->name();

    /* Since it isn't illegal to have elements with children of the same name,
     * we cannot block it in the DB, however, if we DO have elements with
     * children of the same name, we don't want the user to delete the element
     * since bad things will happen.
      */
    if (m_currentElement == m_currentElementParent) {
      ui->deleteElementButton->setEnabled(false);
    } else {
      ui->deleteElementButton->setEnabled(true);
    }

    QStringList attributes =
        DB::instance()->attributes(m_currentElement);

    ui->comboBox->clear();
    ui->comboBox->addItems(attributes);
  }
}

/*----------------------------------------------------------------------------*/

void RemoveItemsForm::attributeActivated(const QString &attribute) {
  m_currentAttribute = attribute;
  QStringList attributeValues = DB::instance()->attributeValues(
      m_currentElement, m_currentAttribute);

  ui->plainTextEdit->clear();

  foreach(QString value, attributeValues) {
    ui->plainTextEdit->insertPlainText(QString("%1\n").arg(value));
  }
}

/*----------------------------------------------------------------------------*/

void RemoveItemsForm::deleteElement(const QString &element) {
  /* If the element name is empty, then this function was called directly by the
   * user clicking on "delete" (as opposed to this function being called further
   * down below during the recursive process of getting rid of the element's
   * children) in that case, the first element to be removed is the current one
   * (set in "elementSelected"). */
  QString currentElement = (element.isEmpty()) ? m_currentElement : element;

  QStringList children =
      DB::instance()->children(currentElement);
  m_deletedElements.clear();

  /* Attributes and values must be removed before we can remove elements and we
   * must also ensure that children are removed before their parents.  To
   * achieve this, we need to ensure that we clean the element tree from "the
   * bottom up". */
  if (!children.isEmpty()) {
    foreach(QString child, children) {
      if (DB::instance()->isUniqueChildElement(currentElement,
                                                              child)) {
        deleteElement(child);
      }
    }
  }

  /* Remove all the attributes (and their known values) associated with this
   * element. */
  QStringList attributes =
      DB::instance()->attributes(currentElement);

  foreach(QString attribute, attributes) {
    if (!DB::instance()->removeAttribute(currentElement,
                                                        attribute)) {
      MessageSpace::showErrorMessageBox(
          this, DB::instance()->lastError());
    }
  }

  /* Now we can remove the element itself. */
  if (!DB::instance()->removeElement(currentElement)) {
    MessageSpace::showErrorMessageBox(
        this, DB::instance()->lastError());
  } else {
    m_deletedElements.append(currentElement);
  }

  /* Check if the user removed a root element. */
  if (DB::instance()->knownRootElements().contains(
          currentElement)) {
    if (!DB::instance()->removeRootElement(currentElement)) {
      MessageSpace::showErrorMessageBox(
          this, DB::instance()->lastError());
    }
  }

  /* Remove the element from all its parents' first level child lists. */
  updateChildLists();

  ui->comboBox->clear();
  ui->plainTextEdit->clear();
  ui->treeWidget->populateFromDatabase();
}

/*----------------------------------------------------------------------------*/

void RemoveItemsForm::removeChildElement() {
  if (!m_currentElementParent.isEmpty()) {
    if (!DB::instance()->removeChildElement(
            m_currentElementParent, m_currentElement)) {
      MessageSpace::showErrorMessageBox(
          this, DB::instance()->lastError());
    } else {
      if (DB::instance()->isUniqueChildElement(
              m_currentElementParent, m_currentElement)) {
        bool accepted = MessageSpace::userAccepted(
            "RemoveUnlistedElement", "Element not used",
            QString("\"%1\" is not assigned to any other element (i.e. "
                    "it isn't used anywhere else in the profile).\n"
                    "Would you like to remove the element completely?")
                .arg(m_currentElement),
            MessageSpace::YesNo, MessageSpace::No, MessageSpace::Question);

        if (accepted) {
          deleteElement();
        }
      }

      ui->comboBox->clear();
      ui->plainTextEdit->clear();
      ui->treeWidget->populateFromDatabase();
    }
  }
}

/*----------------------------------------------------------------------------*/

void RemoveItemsForm::updateAttributeValues() {
  QStringList attributes = ui->plainTextEdit->toPlainText().split("\n");
  attributes.removeAll("");

  if (attributes.isEmpty()) {
    bool accepted = MessageSpace::userAccepted(
        "UpdateEmptyAttributeValues", "Update with empty attribute values?",
        "All known values were removed. "
        "Would you like to remove the attribute completely?",
        MessageSpace::YesNo, MessageSpace::No, MessageSpace::Question);

    if (accepted) {
      deleteAttribute();
    }
  } else {
    /* All existing values will be replaced with whatever remained in the text
      edit by the time the user was done. */
    if (!DB::instance()->updateAttributeValues(
            m_currentElement, m_currentAttribute, attributes, true)) {
      MessageSpace::showErrorMessageBox(
          this, DB::instance()->lastError());
    } else {
      QMessageBox::information(this, "Success", "Done!");
    }
  }
}

/*----------------------------------------------------------------------------*/

void RemoveItemsForm::deleteAttribute() {
  if (!DB::instance()->removeAttribute(m_currentElement,
                                                      m_currentAttribute)) {
    MessageSpace::showErrorMessageBox(
        this, DB::instance()->lastError());
  } else {
    /* Purely for cosmetic effect - updates the tree item to reflect the correct
      node text when in "verbose" mode. */
    ui->treeWidget->CurrentItem()->excludeAttribute(m_currentAttribute);

    ui->comboBox->removeItem(ui->comboBox->findText(m_currentAttribute));
  }
}

/*----------------------------------------------------------------------------*/

void RemoveItemsForm::updateChildLists() {
  QStringList knownElements = DB::instance()->knownElements();

  foreach(QString element, knownElements) {
    QStringList children = DB::instance()->children(element);

    if (!children.isEmpty()) {
      foreach(QString deletedElement, m_deletedElements) {
        if (children.contains(deletedElement)) {
          if (!DB::instance()->removeChildElement(
                  element, deletedElement)) {
            MessageSpace::showErrorMessageBox(
                this, DB::instance()->lastError());
          }
        }
      }
    }
  }
}

/*----------------------------------------------------------------------------*/

void RemoveItemsForm::showElementHelp() {
  QMessageBox::information(
      this, "How this works...",
      "\"Remove Child\" will remove the currently highlighted element "
      "from its parent element's child list, i.e. it will only "
      "affect the relationship between the two elements, the element "
      "itself is not deleted in the process and will remain in the profile. "
      "\n\n"
      "\"Delete Element\" will delete the element, the element's children, "
      "the children's children (etc, etc), its associated attributes, the "
      "associated attributes of its children (and their children, etc, etc), "
      "all "
      "the known values for all the attributes thus deleted and finally also "
      "remove the element (and its children and the children's children, etc "
      "etc) "
      "from every single child list that contains it.\n\n"
      "None of this can be undone. ");
}

/*----------------------------------------------------------------------------*/

void RemoveItemsForm::showAttributeHelp() {
  QMessageBox::information(
      this, "How this works...",
      "\"Delete Attribute\" will also delete all its known values.\n\n"
      "\"Update Attribute Values\" - Only those values remaining in the text "
      "edit when "
      "\"Update Attribute Values\" is clicked will be saved against the "
      "attribute shown "
      "in the drop down (this effectively means that you could also add new "
      "values "
      "to the attribute if you wish).  Just make sure that all the values you "
      "want to "
      "associate with the attribute when you're done appear on separate "
      "lines.");
}

/*----------------------------------------------------------------------------*/
