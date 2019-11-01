/*
    Copyright 2011 Thomas Schöps
    
    This file is part of OpenOrienteering's scoring tool.

    OpenOrienteering's scoring tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenOrienteering's scoring tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenOrienteering's scoring tool.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "problemWidget.h"

#include <QtWidgets>

Problem::Problem()
{
	selection = 0;
}

ProblemWidget::ProblemWidget(QWidget* parent) : QTableWidget(parent)
{
	verticalHeader()->setVisible(false);
	setColumnCount(2);
	QStringList labels;
	labels << tr("Description") << tr("Action");
	setHorizontalHeaderLabels(labels);
	horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	horizontalHeader()->setSectionsClickable(false);
	
	actionSelectionDelegate = new ActionSelectionDelegate();
	setItemDelegateForColumn(1, actionSelectionDelegate);
	
	setEditTriggers(QAbstractItemView::AllEditTriggers);
}
ProblemWidget::~ProblemWidget()
{
	delete actionSelectionDelegate;
	
	int size = problems.size();
	for (int i = 0; i < size; ++i)
		delete problems[i];
}
QSize ProblemWidget::sizeHint() const
{
    return QSize(1024, 500);
}
void ProblemWidget::addProblem(Problem* problem)
{
	int row = problems.size();
	problems.push_back(problem);
	
	setRowCount(rowCount() + 1);
	
	QTableWidgetItem* descItem = new QTableWidgetItem(problem->getDescription());
	descItem->setFlags(Qt::ItemIsEnabled);
	setItem(row, 0, descItem);
	
	QTableWidgetItem* actionItem = new QTableWidgetItem(problem->getSolutionDescription(problem->getSelection()));
	actionItem->setData(Qt::UserRole, qVariantFromValue<void*>(problem));
	setItem(row, 1, actionItem);
}
void ProblemWidget::applyAllSolutions()
{
	int size = problems.size();
	for (int i = 0; i < size; ++i)
		problems[i]->applySolution(reinterpret_cast<Problem*>(item(i, 1)->data(Qt::UserRole).value<void*>())->getSelection());
}
void ProblemWidget::updateAllSolutions()
{
	clearSelection();
	int size = problems.size();
	for (int i = 0; i < size; ++i)
		item(i, 1)->setText(problems[i]->getSolutionDescription(problems[i]->getSelection()));
}

void ProblemWidget::showAsDialog(const QString& title, QWidget* parent)
{
	ProblemDialog problemDialog(title, this, parent);
	problemDialog.setWindowModality(Qt::WindowModal);
	problemDialog.exec();
}

void ProblemWidget::setAllToNo()
{
	int size = problems.size();
	for (int i = 0; i < size; ++i)
		problems[i]->setToSolutionType(Problem::SolutionType_No);
	updateAllSolutions();
}
void ProblemWidget::setAllToKeep()
{
	int size = problems.size();
	for (int i = 0; i < size; ++i)
		problems[i]->setToSolutionType(Problem::SolutionType_Keep);
	updateAllSolutions();
}
void ProblemWidget::setAllToChange()
{
	int size = problems.size();
	for (int i = 0; i < size; ++i)
		problems[i]->setToSolutionType(Problem::SolutionType_Change);
	updateAllSolutions();
}

ProblemDialog::ProblemDialog(const QString& title, ProblemWidget* widget, QWidget* parent) : QDialog(parent), widget(widget)
{
	setWindowTitle(title);
	setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
	setSizeGripEnabled(true);
	
	// TODO: Should the buttons be moved into ProblemWidget?
	QToolButton* setAllButton = new QToolButton();
	setAllButton->setPopupMode(QToolButton::InstantPopup);
	setAllButton->setText(tr("Set all to ..."));
	QMenu* setAllMenu = new QMenu(setAllButton);
	setAllMenu->addAction(tr("No"), widget, SLOT(setAllToNo()));
	setAllMenu->addAction(tr("Yes, keep existing"), widget, SLOT(setAllToKeep()));
	setAllMenu->addAction(tr("Yes, change to new"), widget, SLOT(setAllToChange()));
	setAllButton->setMenu(setAllMenu);
	setAllButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
	
	QPushButton* okButton = new QPushButton(tr("Apply"));
	
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	buttonLayout->addWidget(setAllButton);
	buttonLayout->addStretch(1);
	buttonLayout->addWidget(okButton);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(widget);
	layout->addLayout(buttonLayout);
	setLayout(layout);
	
	connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
}
void ProblemDialog::okClicked()
{
	widget->applyAllSolutions();
	close();
}

// ### ActionSelectionDelegate ###

ActionSelectionDelegate::ActionSelectionDelegate(QObject* parent) : QItemDelegate(parent)
{
}

QWidget* ActionSelectionDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QComboBox* combo = new QComboBox(parent);
	
	Problem* problem = reinterpret_cast<Problem*>(index.data(Qt::UserRole).value<void*>());
	int numActions = problem->getNumSolutions();
	for (int i = 0; i < numActions; ++i)
		combo->addItem(problem->getSolutionDescription(i), i);
	
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(emitCommitData()));
	return combo;
}

void ActionSelectionDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	QComboBox* combo = static_cast<QComboBox*>(editor);
	QString value = index.model()->data(index, Qt::EditRole).toString();
	
	int pos = combo->findText(value, Qt::MatchExactly);
	combo->setCurrentIndex(pos);
}

void ActionSelectionDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	QComboBox* combo = static_cast<QComboBox*>(editor);
	
	model->setData(index, combo->currentText(), Qt::EditRole);
	reinterpret_cast<Problem*>(index.data(Qt::UserRole).value<void*>())->setSelection(combo->itemData(combo->currentIndex(), Qt::UserRole).toInt());
}

void ActionSelectionDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	editor->setGeometry(option.rect);
}

void ActionSelectionDelegate::emitCommitData()
{
	emit commitData(qobject_cast<QWidget*>(sender()));
}
