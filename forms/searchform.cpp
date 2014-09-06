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

#include "searchform.h"
#include "ui_searchform.h"
#include "utils/treewidgetitem.h"

#include <QMessageBox>
#include <QTextBlock>

/*-------------------------------- NON MEMBER FUNCTIONS
 * --------------------------------*/

bool lessThan(TreeWidgetItem *lhs, TreeWidgetItem *rhs) {
  return (lhs->index() < rhs->index());
}

/*----------------------------------------------------------------------------*/

bool greaterThan(TreeWidgetItem *lhs, TreeWidgetItem *rhs) {
  return (lhs->index() > rhs->index());
}

/*---------------------------------- MEMBER FUNCTIONS
 * ----------------------------------*/

SearchForm::SearchForm(const QList<TreeWidgetItem *> &items,
                       QPlainTextEdit *textEdit, QWidget *parent)
    : QDialog(parent), ui(new Ui::SearchForm), m_text(textEdit),
      m_savedBackground(), m_savedForeground(), m_wasFound(false),
      m_searchUp(false), m_firstRun(true), m_previousIndex(-1),
      m_searchFlags(0), m_items(items) {
  ui->setupUi(this);
  ui->lineEdit->setFocus();
  // m_text->setText( docContents );

  connect(ui->searchButton, SIGNAL(clicked()), this, SLOT(search()));
  connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));

  connect(ui->caseSensitiveCheckBox, SIGNAL(clicked()), this,
          SLOT(caseSensitive()));
  connect(ui->wholeWordsCheckBox, SIGNAL(clicked()), this, SLOT(wholeWords()));
  connect(ui->searchUpCheckBox, SIGNAL(clicked()), this, SLOT(searchUp()));

  setAttribute(Qt::WA_DeleteOnClose);
}

/*----------------------------------------------------------------------------*/

SearchForm::~SearchForm() {
  /* The QDomDocument m_doc points at is owned externally. */
  delete ui;
}

/*----------------------------------------------------------------------------*/

void SearchForm::search() {
  m_firstRun = false;
  QString searchText = ui->lineEdit->text();
  bool found = m_text->find(searchText, m_searchFlags);

  /* The first time we enter this function, if the text does not exist
    within the document, "found" and "m_wasFound" will both be false.
    However, if the text does exist, we wish to know that we found a match
    at least once so that, when we reach the end of the document and "found"
    is once more false, we can reset all indices and flags in order to start
    again from the beginning. */
  if (((found != m_wasFound) && m_wasFound) || (!m_wasFound && m_searchUp)) {
    resetCursor();
    found = m_text->find(searchText, m_searchFlags);
  }

  if (found) {
    m_wasFound = true;

    /* Highlight the entire node (element, attributes and attribute values)
      in which the match was found. */
    m_text->moveCursor(QTextCursor::StartOfLine);
    m_text->moveCursor(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

    /* Find all tree widget items whose corresponding element node matches the
      highlighted text. */
    QString nodeText = m_text->textCursor().selectedText().trimmed();
    QList<TreeWidgetItem *> matchingItems = gatherMatchingItems(nodeText);

    if (!matchingItems.empty()) {
      if (!m_searchUp) {
        /* Sort ascending. */
        qSort(matchingItems.begin(), matchingItems.end(), lessThan);
        findMatchingTreeItem(matchingItems, true);
      } else {
        /* Sort descending. */
        qSort(matchingItems.begin(), matchingItems.end(), greaterThan);
        findMatchingTreeItem(matchingItems, false);
      }
    } else {
      highlightFind();
    }
  } else {
    QMessageBox::information(
        this, "Not Found",
        QString("Can't find the text:\"%1\"").arg(searchText));
  }
}

/*----------------------------------------------------------------------------*/

void SearchForm::resetCursor() {
  /* Reset cursor so that we may keep cycling through the document content. */
  if (ui->searchUpCheckBox->isChecked()) {
    m_text->moveCursor(QTextCursor::End);
    QMessageBox::information(this, "Reached Top",
                             "Search reached top, continuing at bottom.");
    m_previousIndex = 9999999;
  } else {
    m_text->moveCursor(QTextCursor::Start);
    QMessageBox::information(this, "Reached Bottom",
                             "Search reached bottom, continuing at top.");
    m_previousIndex = -1;
  }
}

/*----------------------------------------------------------------------------*/

void SearchForm::searchUp() {
  /* If the user ticks the "Search Up" box before anything else, we need to set
    the previous index to a large value to ensure we start at the very bottom. */
  if (m_firstRun) {
    m_previousIndex = 9999999;
  }

  if (ui->searchUpCheckBox->isChecked()) {
    m_searchFlags |= QTextDocument::FindBackward;
    m_searchUp = true;
  } else {
    m_searchFlags ^= QTextDocument::FindBackward;
    m_searchUp = false;
  }
}

/*----------------------------------------------------------------------------*/

void SearchForm::caseSensitive() {
  /* Reset found flag every time the user changes the search options. */
  m_wasFound = false;

  if (ui->caseSensitiveCheckBox->isChecked()) {
    m_searchFlags |= QTextDocument::FindCaseSensitively;
  } else {
    m_searchFlags ^= QTextDocument::FindCaseSensitively;
  }
}

/*----------------------------------------------------------------------------*/

void SearchForm::wholeWords() {
  /* Reset found flag every time the user changes the search options. */
  m_wasFound = false;

  if (ui->wholeWordsCheckBox->isChecked()) {
    m_searchFlags |= QTextDocument::FindWholeWords;
  } else {
    m_searchFlags ^= QTextDocument::FindWholeWords;
  }
}

/*----------------------------------------------------------------------------*/

QList<TreeWidgetItem *>
SearchForm::gatherMatchingItems(const QString &nodeText) {
  QList<TreeWidgetItem *> matchingItems;

  for (int i = 0; i < m_items.size(); ++i) {
    TreeWidgetItem *treeItem = m_items.at(i);

    if (treeItem->toString() == nodeText) {
      matchingItems.append(treeItem);
    }
  }

  return matchingItems;
}

/*----------------------------------------------------------------------------*/

void
SearchForm::findMatchingTreeItem(const QList<TreeWidgetItem *> matchingItems,
                                 bool ascending) {
  for (int i = 0; i < matchingItems.size(); ++i) {
    TreeWidgetItem *treeItem = matchingItems.at(i);

    if ((ascending && treeItem->index() > m_previousIndex) ||
        (!ascending && treeItem->index() < m_previousIndex)) {
      m_previousIndex = treeItem->index();
      resetHighlights();
      emit foundItem(treeItem);
      break;
    }
  }

  if (ui->searchButton->text() == "Search") {
    ui->searchButton->setText("Next");
  }
}

/*----------------------------------------------------------------------------*/

void SearchForm::resetHighlights() {
  QList<QTextEdit::ExtraSelection> extras = m_text->extraSelections();

  for (int i = 0; i < extras.size(); ++i) {
    extras[i].format.setProperty(QTextFormat::FullWidthSelection, true);
    extras[i].format.setBackground(m_savedBackground);
    extras[i].format.setForeground(m_savedForeground);
  }

  m_text->setExtraSelections(extras);
}

/*----------------------------------------------------------------------------*/

void SearchForm::highlightFind() {
  /* First we reset all previous selections. */
  resetHighlights();

  /* Now we can set the highlighted text. */
  m_savedBackground = m_text->textCursor().blockCharFormat().background();
  m_savedForeground = m_text->textCursor().blockCharFormat().foreground();

  QTextEdit::ExtraSelection extra;
  extra.cursor = m_text->textCursor();
  extra.format.setBackground(QApplication::palette().highlight());
  extra.format.setForeground(QApplication::palette().highlightedText());

  QList<QTextEdit::ExtraSelection> extras;
  extras << extra;
  m_text->setExtraSelections(extras);
}

/*----------------------------------------------------------------------------*/
