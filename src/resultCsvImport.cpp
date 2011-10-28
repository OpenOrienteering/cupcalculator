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


#include "resultCsvImport.h"

#include <QtGui>

#include "global.h"

CSVImportDialog::CSVImportDialog(CSVFile* file, QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), file(file)
{
	setWindowTitle(tr("CSV Import"));
	setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
	
	// TODO: support more languages
	const int NUM_LANGUAGES = 3;	// German, English, Italian
	QString firstNameList[] = {"Vorname", "First name", "Nome"};
	QString lastNameList[] = {"Nachname", "Surname", "Cognome"};
	QString yearList[] = {"Jg", "YB", "AN"};
	QString genderList[] = {"G", "S", "S"};
	// TODO: Is always "M" used for male? See mainWindow.cpp ...
	QString timeList[] = {"Zeit", "Time", "Tempo"};
	QString stateList[] = {"Wertung", "Classifier", "Stato classifica"};
	QString clubAbbreviationList[] = {"Abk", "Cl.name", "Sigla"};
	QString clubLocationList[] = {"Ort", "City", "Luogo"};
	QString categoryList[] = {"Lang", "Long", "Lungo"};
	QString notClassifiedList[] = {"AK", "nc", "NC"};
	
	// Find out language
	int lang = 0;
	for (; lang < NUM_LANGUAGES; ++lang)
	{
		int idTest = file->getColumnIndex(firstNameList[lang]);
		if (idTest >= 0)
			break;
	}
	bool foundLanguage = lang < NUM_LANGUAGES;
	
	// Build UI
	QLabel* descLabel = new QLabel(tr("Choose columns to load the data from. Red rows are required."));
	
	signal_mapper = new QSignalMapper(this);
	connect(signal_mapper, SIGNAL(mapped(int)), this, SLOT(columnChanged(int)));
	
	current_row = 0;
	rows_layout = new QGridLayout();
	addRow(&colFirstName, tr("First name"), true, foundLanguage, firstNameList[lang]);
	addRow(&colLastName, tr("Last name"), true, foundLanguage, lastNameList[lang]);
	addRow(&colYear, tr("Year"), false, foundLanguage, yearList[lang]);
	addRow(&colGender, tr("Gender"), true, foundLanguage, genderList[lang]);
	addRow(&colTime, tr("Time"), true, foundLanguage, timeList[lang]);
	addRow(&colState, tr("Result"), false, foundLanguage, stateList[lang]);
	addRow(&colClubPart1, tr("Club name part 1"), true, foundLanguage, clubAbbreviationList[lang], &colClubPart1Backup);
	addRow(&colClubPart2, tr("Club name part 2"), false, foundLanguage, clubLocationList[lang], &colClubPart2Backup);
	addRow(&colCategory, tr("Category"), true, foundLanguage, categoryList[lang]);
	addRow(&colNotClassified, tr("Not classified"), false, foundLanguage, notClassifiedList[lang]);
	
	skip_empty_club_rows_combo = new QCheckBox(tr("Skip runners without club information"));
	skip_empty_club_rows = false;
	
	QPushButton* abort_button = new QPushButton(tr("Abort"));
	import_button = new QPushButton(tr("Import"));
	import_button->setDefault(true);
	
	QHBoxLayout* buttonsLayout = new QHBoxLayout();
	buttonsLayout->addWidget(abort_button);
	buttonsLayout->addStretch(1);
	buttonsLayout->addWidget(import_button);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(descLabel);
	layout->addSpacing(16);
	layout->addLayout(rows_layout);
	layout->addSpacing(16);
	layout->addWidget(skip_empty_club_rows_combo);
	layout->addSpacing(16);
	layout->addLayout(buttonsLayout);
	setLayout(layout);
	
	connect(abort_button, SIGNAL(clicked(bool)), this, SLOT(abortClicked()));
	connect(import_button, SIGNAL(clicked(bool)), this, SLOT(importClicked()));
	connect(skip_empty_club_rows_combo, SIGNAL(clicked(bool)), this, SLOT(skipEmptyClubRowsChecked(bool)));
}
CSVImportDialog::~CSVImportDialog()
{
}

void CSVImportDialog::abortClicked()
{
	reject();
}
void CSVImportDialog::importClicked()
{
	size_t num_runners = colFirstName->size();
	if (colLastName->size() != num_runners ||
		(colYear && colYear->size() != num_runners) ||
		colGender->size() != num_runners)
	{
		QMessageBox::warning(this, APP_NAME, tr("The columns for runner information (first / last name, year, gender) must have the same count!"));
		return;
	}
	if (colLastName->size() > 1 && colLastName->size() != colTime->size() - 1)
	{
		QMessageBox::warning(this, APP_NAME, tr("The column count of the time columns must be one higher than the runner count if there are multiple runners per result!"));
		return;
	}
	
	accept();
}
void CSVImportDialog::skipEmptyClubRowsChecked(bool checked)
{
	skip_empty_club_rows = checked;
}
void CSVImportDialog::columnChanged(int id)
{
	*id_vector[id] = reinterpret_cast<CSVFile::ColumnIndexes*>(widget_vector[id]->itemData(widget_vector[id]->currentIndex(), Qt::UserRole).value<void*>());
}

void CSVImportDialog::addRow(CSVFile::ColumnIndexes** id, QString desc, bool required, bool use_default, QString default_item, CSVFile::ColumnIndexes** backup_id)
{
	QLabel* descLabel = new QLabel(desc);
	if (required)
	{
		QPalette redText;
		redText.setColor(QPalette::WindowText, Qt::red);
		descLabel->setPalette(redText);
	}
	
	QComboBox* box = new QComboBox();
	connect(box, SIGNAL(currentIndexChanged(int)), signal_mapper, SLOT(map()));
	signal_mapper->setMapping(box, id_vector.size());
	id_vector.push_back(id);
	widget_vector.push_back(box);
	
	int selection = 0;
	if (!required)
		box->addItem(tr("[none]"), qVariantFromValue<void*>(NULL));
	for (CSVFile::ColumnMap::iterator it = file->beginColumns(); it != file->endColumns(); ++it)
	{
		QString name = it->first;
		if (use_default && name.compare(default_item, Qt::CaseInsensitive) == 0)
			selection = box->count();
		
		if (it->second.size() > 1)
			name += " (" + QString::number(it->second.size()) + ")";
		box->addItem(name, qVariantFromValue<void*>(&it->second));
	}
	box->setCurrentIndex(selection);
	
	rows_layout->addWidget(descLabel, current_row, 0);
	rows_layout->addWidget(box, current_row, 1);
	
	if (backup_id)
	{
		QLabel* backup_label = new QLabel(tr("Backup:"));
		QComboBox* backup_box = new QComboBox();
		backup_box->setToolTip(tr("This column will be used if the normal column(s) are empty"));
		
		connect(backup_box, SIGNAL(currentIndexChanged(int)), signal_mapper, SLOT(map()));
		signal_mapper->setMapping(backup_box, id_vector.size());
		id_vector.push_back(backup_id);
		widget_vector.push_back(backup_box);
		
		int selection = 0;
		backup_box->addItem(tr("[none]"), qVariantFromValue<void*>(NULL));
		for (CSVFile::ColumnMap::iterator it = file->beginColumns(); it != file->endColumns(); ++it)
		{
			QString name = it->first;
			if (it->second.size() > 1)
				name += " (" + QString::number(it->second.size()) + ")";
			backup_box->addItem(name, qVariantFromValue<void*>(&it->second));
		}
		backup_box->setCurrentIndex(selection);
		
		rows_layout->addWidget(backup_label, current_row, 2);
		rows_layout->addWidget(backup_box, current_row, 3);
	}
	
	++current_row;
}

#include "resultCsvImport.moc"
