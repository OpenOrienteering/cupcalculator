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


#include "clubDialog.h"

#include <QtWidgets>
#include <assert.h>

#include "location.h"
#include "club.h"
#include "global.h"
#include "util.h"

ClubDialog::ClubDialog(Club* _club, QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), club(_club)
{
	old_name = club->getName();
	
	QLabel* nameLabel = new QLabel(tr("Club name:"));
	nameEdit = new QLineEdit();
	
	QHBoxLayout* nameLayout = new QHBoxLayout();
	nameLayout->addWidget(nameLabel);
	nameLayout->addWidget(nameEdit);
	
	regionLabel = new QLabel("");
	
	locationView = new QTreeView();
	locationView->header()->hide();
	locationView->setModel(locationDB.getSortModel());
	
	addCountryButton = new QPushButton(QIcon("images/plus.png"), tr("Add country"));
	addStateButton = new QPushButton(QIcon("images/plus.png"), tr("Add state/province"));
	removeLocationButton = new QPushButton(QIcon("images/minus.png"), "");
	clearSelectionButton = new QPushButton(tr("Clear selection"));
	QHBoxLayout* locationButtonsLayout = new QHBoxLayout();
	locationButtonsLayout->addStretch(1);
	locationButtonsLayout->addWidget(addCountryButton);
	locationButtonsLayout->addWidget(addStateButton);
	locationButtonsLayout->addWidget(removeLocationButton);
	locationButtonsLayout->addWidget(clearSelectionButton);
	
	QPushButton* closeButton = new QPushButton(tr("Close"));
	closeButton->setDefault(true);
	QHBoxLayout* buttonsLayout = new QHBoxLayout();
	buttonsLayout->addStretch(1);
	buttonsLayout->addWidget(closeButton);
	
	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addLayout(nameLayout);
	mainLayout->addWidget(regionLabel);
	mainLayout->addWidget(locationView);
	mainLayout->addStretch(1);
	mainLayout->addLayout(locationButtonsLayout);
	mainLayout->addLayout(buttonsLayout);
	setLayout(mainLayout);
	
	updateWindowTitle();
	
	// Set values
	nameEdit->setText(club->getName());
	nameEdit->selectAll();
	setLocationText();
	if (club->getProvince() != NULL)
		selectLocation(club->getProvince());
	else if (club->getCountry() != NULL)
		selectLocation(club->getCountry());
	
	updateActions();
	
	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(nameEdit, SIGNAL(editingFinished()), this, SLOT(nameChanged()));
	connect(addCountryButton, SIGNAL(clicked()), this, SLOT(addCountryClicked()));
	connect(addStateButton, SIGNAL(clicked()), this, SLOT(addStateClicked()));
	connect(removeLocationButton, SIGNAL(clicked()), this, SLOT(removeLocationClicked()));
	connect(clearSelectionButton, SIGNAL(clicked()), this, SLOT(clearSelectionClicked()));
	
	connect(locationView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(locationSelected()));
	connect(&locationDB, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(setLocationText()));
}
ClubDialog::~ClubDialog()
{
}
void ClubDialog::closeEvent(QCloseEvent* event)
{
	// Check if club name is valid and unique
	if (club->getName().isEmpty())
	{
		QMessageBox::warning(this, APP_NAME, tr("Please enter a club name!"));
		event->ignore();
		return;
	}
	
	if (club->getName() != old_name && clubDB.containsName(club->getName(), club))
	{
		QMessageBox::warning(this, APP_NAME, tr("The name is already in use, please choose a unique name and check if you were trying to enter the same club twice!"));
		event->ignore();
		return;
	}
	
	locationDB.saveToFile();	// TODO: could leave this out if nothing has changed
	clubDB.saveToFile();		// TODO: just as here
	event->accept();
}

void ClubDialog::updateWindowTitle()
{
	setWindowTitle(tr("Edit club") + " - " + club->getName());
}
void ClubDialog::nameChanged()
{
	club->setName(nameEdit->text());
	updateWindowTitle();
}
void ClubDialog::setLocationText()
{
	QString text = tr("Location of club:") + " <b>";
	
	if (club->getProvince())
	{
		assert((club->getCountry() != NULL) && "Province set for a club, but no country!");
		text += club->getProvince()->getName() + ", " + club->getCountry()->getName() + "</b>";
	}
	else if (club->getCountry())
		text += club->getCountry()->getName() + "</b>";
	else
		text += tr("not specified") + "</b>";
	
	regionLabel->setText(text);
}
void ClubDialog::locationSelected()
{
	updateActions();
	
	QModelIndex index = locationView->selectionModel()->currentIndex();
	if (!index.isValid())
		club->setLocation(NULL, NULL);
	else if (index.parent() == QModelIndex())
		club->setLocation(locationDB.getItem(locationDB.getSortModel()->mapToSource(index)), NULL);
	else
		club->setLocation(locationDB.getItem(locationDB.getSortModel()->mapToSource(index.parent())), locationDB.getItem(locationDB.getSortModel()->mapToSource(index)));
	
	setLocationText();
}

void ClubDialog::updateActions()
{
	bool enable = !locationView->selectionModel()->selection().isEmpty() && locationView->selectionModel()->currentIndex().isValid();
	removeLocationButton->setEnabled(enable);
	clearSelectionButton->setEnabled(enable);
	addStateButton->setEnabled(enable);
}

void ClubDialog::selectLocation(Location* location)
{
	// TODO: This seems very inefficient - can it be done easier?
	QAbstractItemModel* model = locationView->model();
	
	int num_rows = model->rowCount();
	for (int row = 0; row < num_rows; ++row)
	{
		QModelIndex index = model->index(row, 0);
		Location* index_location = locationDB.getItem(locationDB.getSortModel()->mapToSource(index));
		if (location == index_location)
		{
			locationView->selectionModel()->setCurrentIndex(model->index(row, 0), QItemSelectionModel::ClearAndSelect);
			return;
		}
		
		// Check all children
		int num_children = model->rowCount(index);
		for (int sub_row = 0; sub_row < num_children; ++sub_row)
		{
			QModelIndex sub_index = locationDB.getSortModel()->mapToSource(model->index(sub_row, 0, index));
			Location* sub_index_location = locationDB.getItem(sub_index);
			if (location == sub_index_location)
			{
				locationView->selectionModel()->setCurrentIndex(model->index(sub_row, 0, index), QItemSelectionModel::ClearAndSelect);
				return;
			}
		}
	}
	
	locationView->selectionModel()->clear();
}

void ClubDialog::insertRow(const QString& text, bool second_level)
{
	int row;
	QModelIndex parent;
	QAbstractItemModel* model = &locationDB;
	QSortFilterProxyModel* sortModel = static_cast<QSortFilterProxyModel*>(locationView->model());
	QModelIndex index = sortModel->mapToSource(locationView->selectionModel()->currentIndex());
	
	if (second_level)
	{
		if (index.parent() == QModelIndex())
			parent = index;
		else
			parent = index.parent();
	}
	else
		parent = QModelIndex();
	row = 0;
	
	if (!model->insertRow(row, parent))
		return;
	
	QModelIndex child = model->index(row, 0, parent);
	model->setData(child, QVariant(text), Qt::EditRole);
	
	
	index = sortModel->mapFromSource(model->index(row, 0, parent));
	locationView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
	locationDB.getSortModel()->sort(0);
	locationView->edit(index);
}
void ClubDialog::addCountryClicked()
{
	insertRow(tr("New country"), false);
}
void ClubDialog::addStateClicked()
{
	insertRow(tr("New state/province"), true);
}
void ClubDialog::removeLocationClicked()
{
	QModelIndex index = locationView->selectionModel()->currentIndex();
	QAbstractItemModel* model = locationView->model();
	Location* index_location = locationDB.getItem(locationDB.getSortModel()->mapToSource(index));
	
	if (clubDB.findClubAt(index_location, club) != NULL)
	{
		QMessageBox::warning(this, APP_NAME, tr("The location %1 you want to delete is still referenced by at least one club. You must delete the references first!").arg(index_location->getName()));
		return;
	}
	int num_children = index_location->getChildCount();
	for (int i = 0; i < num_children; ++i)
	{
		Location* sub_location = index_location->getChild(i);
		if (clubDB.findClubAt(sub_location, club) != NULL)
		{
			QMessageBox::warning(this, APP_NAME, tr("The sub-location %1 you want to delete is still referenced by at least one club. You must delete the references first!").arg(sub_location->getName()));
			return;
		}
	}
	
	if (QMessageBox::question(this, APP_NAME, tr("Confirm: delete the location %1?").arg(index_location->getName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
		return;
	
	if (model->removeRow(index.row(), index.parent()))
		updateActions();
}
void ClubDialog::clearSelectionClicked()
{
	locationView->setCurrentIndex(QModelIndex());
}
