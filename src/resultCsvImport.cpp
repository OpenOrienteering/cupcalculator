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

#include <QtWidgets>

#include "global.h"

CSVImportDialog::CSVImportDialog(CSVFile* file, QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), file(file)
{
	setWindowTitle(tr("CSV Import"));
	setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
	
	const int NUM_LANGUAGES = 14;	// German, English, Italian, Swedish, Spanish, Hungarian, Czech, Croatian, French, Polish, Danish, "Bulgarian" (unfinished in OE2003), Finnish, "Estonian" / "Russian" (unfinished in OE2003)
	QString firstNameList[NUM_LANGUAGES] = {"Vorname", "First name", "Nome", "Förnamn", "Nombre", "Utónév", "Jméno (køest.)", "Ime", "Prénom", "Imiê", "Fornavn", "Vorname", "Etunimi", "Vorname"};
	QString lastNameList[NUM_LANGUAGES] = {"Nachname", "Surname", "Cognome", "Efternamn", "Apellidos", "Családnév", "Pøíjmení", "Prezime", "Nom", "Nazwisko", "Efternavn", "Nachname", "Sukunimi", "Nachname"};
	QString yearList[NUM_LANGUAGES] = {"Jg", "YB", "AN", "År", "AN", "Szül", "RN", "GR", "Né", "Ur.", "År", "Jg", "Synt", "Jg"};
	QString genderList[NUM_LANGUAGES] = {"G", "S", "S", "K", "S", "FN", "S", "Spol", "S", "P³eæ", "K", "G_Sex", "S", "G_Sex"};
	QString timeList[NUM_LANGUAGES] = {"Zeit", "Time", "Tempo", "Tid", "Tiempo", "Idõ", "Èas", "Vrijeme", "Temps", "Czas", "Tid", "Âðåìå", "Aika", "Zeit"};
	QString stateList[NUM_LANGUAGES] = {"Wertung", "Classifier", "Stato classifica", "Status", "Estado clas.", "Érvényes", "Klasifikace", "Klasifikator", "Evaluation", "Klasyfikowany", "Status", "Wertung", "Tila", "Wertung"};
	QString clubAbbreviationList[NUM_LANGUAGES] = {"Abk", "Cl.name", "Sigla", "Namn", "Nombre de club", "Név", "Název klubu", "Klub ime", "Nom", "Klub", "Navn", "Abk", "Nimi", "Abk"};
	QString clubLocationList[NUM_LANGUAGES] = {"Ort", "City", "Luogo", "Ort", "Ciudad", "Város", "Mìsto", "Mjesto", "Ville", "Miasto", "Klub", "Ort", "Kunta", "Ort"};
	QString categoryList[NUM_LANGUAGES] = {"Lang", "Long", "Lungo", "Lång", "Largo", "Hosszú", "Dlouhý", "Dugo", "Long", "D³uga nazwa", "Lang", "Lang", "Pitkä", "Lang"};
	QString notClassifiedList[NUM_LANGUAGES] = {"AK", "nc", "NC", "ut", "nc", "Vk", "ms", "NKV", "nc", "pk", "UFK", "AK_notclass", "et", "AK_notclass"};

	// String used to classify male genders inside the table.
	QString maleGenderStringList[NUM_LANGUAGES] = {"M", "M", "U", "M", "H", "F", "H", "M", "H", "M", "M", "M_Male", "M", "M_Male"};
	
	// Find out language
	int lang = 0;
	for (; lang < NUM_LANGUAGES; ++lang)
	{
		int idTest1 = file->getColumnIndex(notClassifiedList[lang]);
		int idTest2 = file->getColumnIndex(timeList[lang]);
		if (idTest1 >= 0 && idTest2 >= 0)
			break;
	}
	bool foundLanguage = lang < NUM_LANGUAGES;
	if (!foundLanguage)
		lang = 0; // prevent invalid accesses later (and default to "M" as string to classify males)

	maleString = maleGenderStringList[lang];

	// Reformat OS2010 files to OE2003 format (at least the columns we are interested in)
	if (file->getColumnIndex("OS0012") >= 0 && foundLanguage)
	{
		const int kColumnsCount = 20;
		bool haveColumn[kColumnsCount];
		for (int i = 0; i < kColumnsCount; ++ i)
			haveColumn[i] = false;
		while (file->nextLine())
		{
			for (int i = 0; i < kColumnsCount; ++ i)
			{
				QString number = QString("%1").arg(i + 1);
				haveColumn[i] |= (file->getColumnIndex(firstNameList[lang] + number) >= 0) && !file->getValue(file->getColumnIndex(firstNameList[lang] + number)).isEmpty();
				haveColumn[i] |= (file->getColumnIndex(lastNameList[lang] + number) >= 0) && !file->getValue(file->getColumnIndex(lastNameList[lang] + number)).isEmpty();
				haveColumn[i] |= (file->getColumnIndex(yearList[lang] + number) >= 0) && !file->getValue(file->getColumnIndex(yearList[lang] + number)).isEmpty();
				haveColumn[i] |= (file->getColumnIndex(genderList[lang] + number) >= 0) && !file->getValue(file->getColumnIndex(genderList[lang] + number)).isEmpty();
				haveColumn[i] |= (file->getColumnIndex(timeList[lang] + number) >= 0) && !file->getValue(file->getColumnIndex(timeList[lang] + number)).isEmpty();
				haveColumn[i] |= (file->getColumnIndex(stateList[lang] + number) >= 0) && !file->getValue(file->getColumnIndex(stateList[lang] + number)).isEmpty();
			}
		}
		file->rewind();
		for (int i = 0; i < kColumnsCount; ++ i)
		{
			QString number = QString("%1").arg(i + 1);
			if (haveColumn[i])
			{
				file->renameColumn(firstNameList[lang] + number, firstNameList[lang]);
				file->renameColumn(lastNameList[lang] + number, lastNameList[lang]);
				file->renameColumn(yearList[lang] + number, yearList[lang]);
				file->renameColumn(genderList[lang] + number, genderList[lang]);
				file->renameColumn(timeList[lang] + number, timeList[lang]);
				file->renameColumn(stateList[lang] + number, stateList[lang]);
			}
			else
			{
				file->deleteColumn(firstNameList[lang] + number);
				file->deleteColumn(lastNameList[lang] + number);
				file->deleteColumn(yearList[lang] + number);
				file->deleteColumn(genderList[lang] + number);
				file->deleteColumn(timeList[lang] + number);
				file->deleteColumn(stateList[lang] + number);
			}
		}
	}
	
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
