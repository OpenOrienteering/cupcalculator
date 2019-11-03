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


#include "seriesScoringDialog.h"

#include <cassert>
#include <map>
#include <set>
#include <utility>

#include <Qt>
#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMap>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QVariant>

#include "config.h"
#include "csvFile.h"
#include "resultList.h"
#include "runner.h"
#include "seriesScoring.h"
#include "util.h"

QT_BEGIN_NAMESPACE
class QHBoxLayout;
class QVBoxLayout;
QT_END_NAMESPACE

// ### SeriesScoringDialog ###

SeriesScoringDialog::SeriesScoringDialog(SeriesScoring* _scoring, QWidget* parent): QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), scoring(_scoring)
{
	old_file_name = scoring->getFileName();
	
	pagesWidget = new QStackedWidget();
	settingsPage = new SeriesSettingsPage(scoring, this);
	racesPage = new SeriesRacesPage(scoring, this);
	resultPage = new SeriesResultsPage(scoring, this);
	pagesWidget->addWidget(settingsPage);
	pagesWidget->addWidget(racesPage);
	pagesWidget->addWidget(resultPage);
	
	stepsList = new QListWidget();
	stepsList->setMaximumWidth(160);
	stepsList->setSpacing(2);
	
	createSteps();
	stepsList->setCurrentRow(0);
	
	QPushButton* closeButton = new QPushButton(tr("Close"));
	
	QHBoxLayout* horizontalLayout = new QHBoxLayout();
	horizontalLayout->addWidget(stepsList);
	horizontalLayout->addWidget(pagesWidget, 1);
	
	QHBoxLayout* buttonsLayout = new QHBoxLayout();
	buttonsLayout->addStretch(1);
	buttonsLayout->addWidget(closeButton);
	
	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addLayout(horizontalLayout, 1);
	mainLayout->addLayout(buttonsLayout);
	setLayout(mainLayout);
	
	updateWindowTitle();
	setSizeGripEnabled(true);
	
	connect(stepsList, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
			this, SLOT(changePage(QListWidgetItem*, QListWidgetItem*)));
	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
}
SeriesScoringDialog::~SeriesScoringDialog()
{
}

void SeriesScoringDialog::jumpToRaceResult(SeriesRaceResult* result)
{
	stepsList->setCurrentRow(1);
	racesPage->jumpToRaceResult(result);
}

void SeriesScoringDialog::createSteps()
{
	next_step_number = 1;
	createStep(tr("Settings"), settingsPage);
	createStep(tr("Races"), racesPage);
	createStep(tr("Result"), resultPage);
}
QListWidgetItem* SeriesScoringDialog::createStep(const QString& text, QWidget* page)
{
	QListWidgetItem* step = new QListWidgetItem(stepsList);
	step->setText(QString::number(next_step_number) + ". " + text);
	step->setTextAlignment(Qt::AlignHCenter);
	step->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	step->setData(Qt::UserRole, qVariantFromValue(page));
	
	++next_step_number;
	
	return step;
}

void SeriesScoringDialog::changePage(QListWidgetItem* current, QListWidgetItem* previous)
{
	if (!current)
		current = previous;
	
	QWidget* pageWidget = current->data(Qt::UserRole).value<QWidget*>();
	if (pageWidget == resultPage)
		resultPage->activated();
	pagesWidget->setCurrentWidget(pageWidget);
}

void SeriesScoringDialog::closeEvent(QCloseEvent* event)
{
	// Check if file name is valid and unique
	if (scoring->getFileName().isEmpty())
	{
		QMessageBox::warning(this, APP_NAME, tr("Please enter a file name!"));
		event->ignore();
		return;
	}
	
	if (scoring->getFileName() != old_file_name)
	{
		if (seriesScoringDB.contains(scoring->getFileName()))
		{
			QMessageBox::warning(this, APP_NAME, tr("The file name is already in use, please choose a unique name!"));
			event->ignore();
			return;
		}
		else
			seriesScoringDB.changeScoringName(old_file_name, scoring->getFileName());
	}
	
	//scoring->saveToFile();
	event->accept();
}
QSize SeriesScoringDialog::sizeHint() const
{
	return QSize(900, 500);
}
void SeriesScoringDialog::updateWindowTitle()
{
	setWindowTitle(tr("Edit series scoring - ") + scoring->getFileName() + ".xml");
}

// ### SeriesSettingsPage ###

SeriesSettingsPage::SeriesSettingsPage(SeriesScoring* _scoring, SeriesScoringDialog* _dialog, QWidget* parent) : QWidget(parent), scoring(_scoring), dialog(_dialog)
{
	QLabel* nameLabel = new QLabel(tr("File name:"));
	nameEdit = new QLineEdit();
	QLabel* nameSuffixLabel = new QLabel(".xml");
	
	QHBoxLayout* nameLayout = new QHBoxLayout();
	nameLayout->addWidget(nameLabel);
	nameLayout->addWidget(nameEdit);
	nameLayout->addWidget(nameSuffixLabel);
	
	only_best_runs_count_check = new QCheckBox(tr("Count only best"));
	countingRunsEdit = new QLineEdit();
	countingRunsEdit->setValidator(new QIntValidator(1, 999999, countingRunsEdit));
	QLabel* only_best_runs_count_label = new QLabel(tr("races", "count"));
	
	QHBoxLayout* countingRunsLayout = new QHBoxLayout();
	countingRunsLayout->addWidget(only_best_runs_count_check);
	countingRunsLayout->addWidget(countingRunsEdit);
	countingRunsLayout->addWidget(only_best_runs_count_label);
	countingRunsLayout->addStretch(1);
	
	QGroupBox* organizerGroup = new QGroupBox(tr("Organizer Bonus"));
	QRadioButton* no_organizer_bonus_radio = new QRadioButton(tr("none"));
	QRadioButton* fixed_organizer_bonus_radio = new QRadioButton(tr("fixed amount of points:"));
	fixedOrganizerBonusAmount = new QLineEdit();
	fixedOrganizerBonusAmount->setValidator(new DoubleValidator(0));
	QRadioButton* percentage_organizer_bonus_radio = new QRadioButton(tr(""));
	percentageOrganizerBonusAmount = new QLineEdit();
	percentageOrganizerBonusAmount->setValidator(new DoubleValidator(0));
	QLabel* percentage_organizer_bonus_label1 = new QLabel(tr("% of point average of best"));
	percentageOrganizerBonusCountingRuns = new QLineEdit();
	percentageOrganizerBonusCountingRuns->setValidator(new QIntValidator(1, 999999, percentageOrganizerBonusCountingRuns));
	QLabel* percentage_organizer_bonus_label2 = new QLabel(tr("races", "point average"));
	
	QHBoxLayout* fixed_organizer_bonus_layout = new QHBoxLayout();
	fixed_organizer_bonus_layout->addWidget(fixed_organizer_bonus_radio);
	fixed_organizer_bonus_layout->addWidget(fixedOrganizerBonusAmount);
	fixed_organizer_bonus_layout->addStretch(1);
	
	QHBoxLayout* percentage_organizer_bonus_layout = new QHBoxLayout();
	percentage_organizer_bonus_layout->addWidget(percentage_organizer_bonus_radio);
	percentage_organizer_bonus_layout->addWidget(percentageOrganizerBonusAmount);
	percentage_organizer_bonus_layout->addWidget(percentage_organizer_bonus_label1);
	percentage_organizer_bonus_layout->addWidget(percentageOrganizerBonusCountingRuns);
	percentage_organizer_bonus_layout->addWidget(percentage_organizer_bonus_label2);
	percentage_organizer_bonus_layout->addStretch(1);
	
	QVBoxLayout* organizer_group_layout = new QVBoxLayout();
	organizer_group_layout->addWidget(no_organizer_bonus_radio);
	organizer_group_layout->addLayout(fixed_organizer_bonus_layout);
	organizer_group_layout->addLayout(percentage_organizer_bonus_layout);
	organizerGroup->setLayout(organizer_group_layout);
	
	organizerButtonGroup = new QButtonGroup();
	organizerButtonGroup->addButton(no_organizer_bonus_radio, (int)NoOrganizerBonus);
	organizerButtonGroup->addButton(fixed_organizer_bonus_radio, (int)FixedOrganizerBonus);
	organizerButtonGroup->addButton(percentage_organizer_bonus_radio, (int)PercentageOrganizerBonus);
	
	// Set values
	nameEdit->setText(scoring->getFileName());
	only_best_runs_count_check->setChecked(scoring->getCountOnlyBestRuns());
	countingRunsEdit->setText(QString::number(scoring->getNumCountingRuns()));
	organizerButtonGroup->button(scoring->getOrganizerBonusType())->setChecked(true);
	fixedOrganizerBonusAmount->setText(scoring->getOrganizerBonusFixed().toString());
	percentageOrganizerBonusAmount->setText(scoring->getOrganizerBonusPercentage().toString());
	percentageOrganizerBonusCountingRuns->setText(QString::number(scoring->getOrganizerBonusCountingRuns()));
	
	updateWidgetActivations();
	
	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addLayout(nameLayout);
	mainLayout->addSpacing(32);
	mainLayout->addLayout(countingRunsLayout);
	mainLayout->addSpacing(8);
	mainLayout->addWidget(organizerGroup);
	mainLayout->addStretch(1);
	setLayout(mainLayout);
	
	connect(nameEdit, SIGNAL(editingFinished()), this, SLOT(nameChanged()));
	connect(organizerButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(organizerBonusTypeChanged(int)));
	connect(only_best_runs_count_check, SIGNAL(clicked(bool)), this, SLOT(onlyBestRunsCountCheckToggled(bool)));
	connect(countingRunsEdit, SIGNAL(editingFinished()), this, SLOT(countingRunsChanged()));
	connect(fixedOrganizerBonusAmount, SIGNAL(editingFinished()), this, SLOT(fixedOrganizerBonusChanged()));
	connect(percentageOrganizerBonusAmount, SIGNAL(editingFinished()), this, SLOT(percentageOrganizerBonusChanged()));
	connect(percentageOrganizerBonusCountingRuns, SIGNAL(editingFinished()), this, SLOT(percentageOrganizerBonusCountingRunsChanged()));
}
void SeriesSettingsPage::nameChanged()
{
	scoring->setFileName(nameEdit->text());
	dialog->updateWindowTitle();
}
void SeriesSettingsPage::organizerBonusTypeChanged(int id)
{
	scoring->setOrganizerBonusType(id);
	updateWidgetActivations();
}
void SeriesSettingsPage::onlyBestRunsCountCheckToggled(bool checked)
{
	scoring->setCountOnlyBestRuns(checked);
	updateWidgetActivations();
}
void SeriesSettingsPage::countingRunsChanged()
{
	scoring->setNumCountingRuns(countingRunsEdit->text().toInt());
}
void SeriesSettingsPage::fixedOrganizerBonusChanged()
{
	scoring->setOrganizerBonusFixed(FPNumber(fixedOrganizerBonusAmount->text().toDouble()));
}
void SeriesSettingsPage::percentageOrganizerBonusChanged()
{
	scoring->setOrganizerBonusPercentage(FPNumber(percentageOrganizerBonusAmount->text().toDouble()));
}
void SeriesSettingsPage::percentageOrganizerBonusCountingRunsChanged()
{
	scoring->setOrganizerBonusCountingRuns(percentageOrganizerBonusCountingRuns->text().toInt());
}
void SeriesSettingsPage::updateWidgetActivations()
{
	countingRunsEdit->setEnabled(scoring->getCountOnlyBestRuns());
	fixedOrganizerBonusAmount->setEnabled(scoring->getOrganizerBonusType() == FixedOrganizerBonus);
	percentageOrganizerBonusAmount->setEnabled(scoring->getOrganizerBonusType() == PercentageOrganizerBonus);
	percentageOrganizerBonusCountingRuns->setEnabled(scoring->getOrganizerBonusType() == PercentageOrganizerBonus);
}

// ### SeriesRacesPage ###

SeriesRacesPage::SeriesRacesPage(SeriesScoring* _scoring, SeriesScoringDialog* _dialog, QWidget* parent) : QWidget(parent), scoring(_scoring), dialog(_dialog)
{
	editOrganizers = false;
	
	QLabel* raceLabel = new QLabel(tr("Race:"));
	racesCombo = new QComboBox();
	changeRaceNumberButton = new QPushButton(tr("Change number"));
	deleteRaceButton = new QPushButton(tr("Delete race"));
	QLabel* nameLabel = new QLabel(tr("Name or description:"));
	nameEdit = new QLineEdit();
	resultsTable = new ResultsTable(nullptr);
	
	QLabel* organizersLabel = new QLabel(tr("Organizers:"));
	organizersList = new QListWidget();
	editOrganizersButton = new QPushButton(tr("Edit"));
	
	QHBoxLayout* raceLayout = new QHBoxLayout();
	raceLayout->addWidget(raceLabel);
	raceLayout->addWidget(racesCombo);
	raceLayout->addWidget(changeRaceNumberButton);
	raceLayout->addWidget(deleteRaceButton);
	
	QHBoxLayout* nameLayout = new QHBoxLayout();
	nameLayout->addWidget(nameLabel);
	nameLayout->addWidget(nameEdit);
	
	QHBoxLayout* organizerButtonsLayout = new QHBoxLayout();
	organizerButtonsLayout->addStretch(1);
	organizerButtonsLayout->addWidget(editOrganizersButton);
	
	// Set values
	for (SeriesScoring::ResultMap::iterator it = scoring->beginResults(); it != scoring->endResults(); ++it)
		racesCombo->addItem(QString::number(it->first) + ". " + it->second->name, qVariantFromValue<void*>(it->second));
	updateResults();
	
	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addLayout(raceLayout);
	mainLayout->addLayout(nameLayout);
	mainLayout->addWidget(resultsTable, 1);
	mainLayout->addSpacing(32);
	mainLayout->addWidget(organizersLabel);
	mainLayout->addWidget(organizersList);
	mainLayout->addLayout(organizerButtonsLayout);
	setLayout(mainLayout);
	
	connect(racesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(currentRaceChanged(int)));
	connect(nameEdit, SIGNAL(editingFinished()), this, SLOT(nameChanged()));
	connect(changeRaceNumberButton, SIGNAL(clicked()), this, SLOT(changeRaceNumberClicked()));
	connect(deleteRaceButton, SIGNAL(clicked()), this, SLOT(deleteRaceClicked()));
	connect(editOrganizersButton, SIGNAL(clicked()), this, SLOT(editOrganizersClicked()));
	connect(organizersList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(currentOrganizerChanged(QListWidgetItem*,QListWidgetItem*)));
	connect(organizersList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(organizerChecked(QListWidgetItem*)));
}
void SeriesRacesPage::jumpToRaceResult(SeriesRaceResult* result)
{
	int index = racesCombo->findData(qVariantFromValue<void*>(result), Qt::UserRole);
	assert(index != -1);
	racesCombo->setCurrentIndex(index);
}
void SeriesRacesPage::changeRaceNumberClicked()
{
	if (racesCombo->currentIndex() == -1)
		return;
	SeriesRaceResult* result = static_cast<SeriesRaceResult*>(racesCombo->itemData(racesCombo->currentIndex(), Qt::UserRole).value<void*>());
	
	ChangeRaceNumberDialog dialog(scoring, result->name, this);
	dialog.setWindowModality(Qt::WindowModal);
	dialog.exec();
	if (dialog.result() == QDialog::Rejected)
		return;
	
	scoring->changeResultNumber(scoring->getRaceResultNumber(result), dialog.getNumber());
	updateRaceResultDisplay();
}
void SeriesRacesPage::deleteRaceClicked()
{
	if (racesCombo->currentIndex() == -1)
		return;
	SeriesRaceResult* result = static_cast<SeriesRaceResult*>(racesCombo->itemData(racesCombo->currentIndex(), Qt::UserRole).value<void*>());
	if (QMessageBox::question(this, APP_NAME, tr("Confirm: delete the race %1?").arg(result->name), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
		return;
	
	scoring->deleteResult(result);
	racesCombo->clear();
	for (SeriesScoring::ResultMap::iterator it = scoring->beginResults(); it != scoring->endResults(); ++it)
		racesCombo->addItem(QString::number(it->first) + ". " + it->second->name, qVariantFromValue<void*>(it->second));
	updateResults();
}
void SeriesRacesPage::editOrganizersClicked()
{
	editOrganizers = !editOrganizers;
	
	organizersList->clear();
	if (editOrganizers)
		showEditOrganizersList();
	else
		showOrganizers();
}
void SeriesRacesPage::showEditOrganizersList()
{
	SeriesRaceResult* result = static_cast<SeriesRaceResult*>(racesCombo->itemData(racesCombo->currentIndex(), Qt::UserRole).value<void*>());
	
	disconnect(organizersList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(organizerChecked(QListWidgetItem*)));
	int size = runnerDB.rowCount();
	for (int i = 0; i < size; ++i)
	{
		Runner* runner = runnerDB.getItem(runnerDB.index(i, 0));
		bool checked = (result->organizers.find(runner) != result->organizers.end());
		
		QListWidgetItem* item = new QListWidgetItem(organizersList);
		item->setText(runner->getShortDesc());
		item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
		item->setData(Qt::UserRole, qVariantFromValue<void*>(runner));
	}
	connect(organizersList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(organizerChecked(QListWidgetItem*)));

	editOrganizersButton->setText(tr("Done"));
}
void SeriesRacesPage::showOrganizers()
{
	SeriesRaceResult* result = static_cast<SeriesRaceResult*>(racesCombo->itemData(racesCombo->currentIndex(), Qt::UserRole).value<void*>());
	
	for (std::set< Runner* >::iterator it = result->organizers.begin(); it != result->organizers.end(); ++it)
	{
		QListWidgetItem* item = new QListWidgetItem(organizersList);
		item->setText((*it)->getShortDesc());
		item->setFlags(Qt::ItemIsEnabled);
	}
	
	editOrganizersButton->setText(tr("Edit"));
}
void SeriesRacesPage::organizerChecked(QListWidgetItem* item)
{
	SeriesRaceResult* result = static_cast<SeriesRaceResult*>(racesCombo->itemData(racesCombo->currentIndex(), Qt::UserRole).value<void*>());
	Runner* runner = static_cast<Runner*>(item->data(Qt::UserRole).value<void*>());
	
	if (item->checkState() == Qt::Checked)
		result->organizers.insert(runner);
	else
		result->organizers.erase(runner);
	scoring->setResultDirty();
}
void SeriesRacesPage::currentOrganizerChanged(QListWidgetItem* /*current*/, QListWidgetItem* /*previous*/)
{
	//removeOrganizerButton->setEnabled(current != nullptr);
}
void SeriesRacesPage::updateRaceResultDisplay()
{
	if (racesCombo->currentIndex() == -1)
		return;
	SeriesRaceResult* result = static_cast<SeriesRaceResult*>(racesCombo->itemData(racesCombo->currentIndex(), Qt::UserRole).value<void*>());
	racesCombo->setItemText(racesCombo->currentIndex(), QString::number(scoring->getRaceResultNumber(result)) + ". " + result->name);
}
void SeriesRacesPage::nameChanged()
{
	if (racesCombo->currentIndex() == -1)
		return;
	SeriesRaceResult* result = static_cast<SeriesRaceResult*>(racesCombo->itemData(racesCombo->currentIndex(), Qt::UserRole).value<void*>());
	result->name = nameEdit->text();
	updateRaceResultDisplay();
	scoring->setResultDirty();
}
void SeriesRacesPage::currentRaceChanged(int /*index*/)
{
	updateResults();
}
void SeriesRacesPage::updateResults()
{
	bool raceSelected = racesCombo->currentIndex() != -1;
	organizersList->setEnabled(raceSelected);
	organizersList->clear();
	resultsTable->setEnabled(raceSelected);
	resultsTable->setModel(nullptr);
	nameEdit->setEnabled(raceSelected);
	nameEdit->setText("");
	editOrganizersButton->setEnabled(raceSelected);
	changeRaceNumberButton->setEnabled(raceSelected);
	deleteRaceButton->setEnabled(raceSelected);
	
	if (racesCombo->currentIndex() == -1)
		return;
	
	SeriesRaceResult* result = static_cast<SeriesRaceResult*>(racesCombo->itemData(racesCombo->currentIndex(), Qt::UserRole).value<void*>());
	
	nameEdit->setText(result->name);
	resultsTable->setModel(result->list);
	
	editOrganizers = false;
	showOrganizers();
}

// ### SeriesResultsPage ###

SeriesResultsPage::SeriesResultsPage(SeriesScoring* _scoring, SeriesScoringDialog* _dialog, QWidget* parent) : QWidget(parent), scoring(_scoring), dialog(_dialog)
{
	recalculateButton = new QPushButton(QIcon("images/refresh.png"), tr("(Re)calculate results"));
	notUpToDateLabel = new QLabel(tr("Results are not up to date. Click the button above to update them."));
	QPalette redText;
	redText.setColor(QPalette::WindowText, Qt::red);
	notUpToDateLabel->setPalette(redText);
	if (!scoring->isResultDirty())
		notUpToDateLabel->hide();
	results = new ResultsTable(scoring->getResult());
	exportButton = new QPushButton(QIcon("images/save.png"), tr("Export results ..."));
	
	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addWidget(recalculateButton);
	mainLayout->addWidget(notUpToDateLabel);
	mainLayout->addWidget(results);
	mainLayout->addWidget(exportButton);
	setLayout(mainLayout);
	
	exportButton->setEnabled(scoring->getResult() != nullptr);
	
	connect(recalculateButton, SIGNAL(clicked()), this, SLOT(recalculateClicked()));
	connect(exportButton, SIGNAL(clicked()), this, SLOT(exportClicked()));
}
void SeriesResultsPage::activated()
{
	notUpToDateLabel->setVisible(scoring->isResultDirty());
	recalculateButton->setEnabled(scoring->isResultDirty());
}
void SeriesResultsPage::recalculateClicked()
{
	scoring->recalculateResult();
	results->setModel(scoring->getResult());
	
	recalculateButton->setEnabled(false);
	notUpToDateLabel->hide();
	exportButton->setEnabled(scoring->getResult() != nullptr);
}
void SeriesResultsPage::exportClicked()
{
	assert(scoring->getResult() != nullptr);
	
	QString path = QFileDialog::getSaveFileName(this, tr("Export scoring ..."), QString(), tr("CSV files (*.csv);;All files (*.*)"));
	if (path.isEmpty())
		return;
	if (!path.endsWith(".csv", Qt::CaseInsensitive))
		path += ".csv";
	
	CSVFile file(path);
	if (!file.showExportDialog(window()))
		return;
	if (!file.open(false))
	{
		QMessageBox::warning(this, APP_NAME, tr("Could not open file:\n") + path);
		return;
	}
	scoring->getResult()->exportToCSV(&file);
	file.close();
	
	QMessageBox::information(this, APP_NAME, tr("Export successful."));
}

// ### AddToSeriesScoringDialog ###

AddToSeriesScoringDialog::AddToSeriesScoringDialog(ResultList* _result_list, QWidget* parent) : QDialog(parent), result_list(_result_list)
{
	new_result = nullptr;
	added_to_scoring = nullptr;
	
	setWindowTitle(tr("Add race results to series scoring"));
	
	QLabel* seriesLabel = new QLabel(tr("Series:"));
	seriesCombo = new QComboBox();
	racesList = new QListWidget();
	QLabel* addLabel = new QLabel(tr("Add as race with number:"));
	raceNumberEdit = new QLineEdit();
	raceNumberEdit->setValidator(new QIntValidator(1, 999999, raceNumberEdit));
	QLabel* nameLabel = new QLabel(tr("Name or description:"));
	nameEdit = new QLineEdit();
	statusLabel = new QLabel(tr("Cannot add the result to this series scoring because the result layouts do not match!\nOne result contains individual runners, the other one clubs."));
	QPalette redText;
	redText.setColor(QPalette::WindowText, Qt::red);
	statusLabel->setPalette(redText);
	statusLabel->hide();
	QPushButton* closeButton = new QPushButton(tr("Cancel"));
	addButton = new QPushButton(QIcon("images/plus.png"), tr("Add"));
	
	QHBoxLayout* seriesLayout = new QHBoxLayout();
	seriesLayout->addWidget(seriesLabel);
	seriesLayout->addWidget(seriesCombo);
	
	QHBoxLayout* raceNumberLayout = new QHBoxLayout();
	raceNumberLayout->addWidget(addLabel);
	raceNumberLayout->addWidget(raceNumberEdit);
	
	QHBoxLayout* nameLayout = new QHBoxLayout();
	nameLayout->addWidget(nameLabel);
	nameLayout->addWidget(nameEdit);
	
	QHBoxLayout* buttonsLayout = new QHBoxLayout();
	buttonsLayout->addWidget(closeButton);
	buttonsLayout->addStretch(1);
	buttonsLayout->addWidget(addButton);
	
	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addLayout(seriesLayout);
	mainLayout->addWidget(racesList);
	mainLayout->addLayout(raceNumberLayout);
	mainLayout->addLayout(nameLayout);
	mainLayout->addStretch(1);
	mainLayout->addWidget(statusLabel);
	mainLayout->addLayout(buttonsLayout);
	setLayout(mainLayout);
	
	// Set values
	for (SeriesScoringDB::Scorings::iterator it = seriesScoringDB.begin(); it != seriesScoringDB.end(); ++it)
		seriesCombo->addItem(it.key(), qVariantFromValue<void*>(*it));
	currentSeriesChanged(seriesCombo->currentIndex());
	
	connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));
	connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
	connect(seriesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(currentSeriesChanged(int)));
}
void AddToSeriesScoringDialog::currentSeriesChanged(int index)
{
	if (index == -1)
		return;
	
	SeriesScoring* scoring = static_cast<SeriesScoring*>(seriesCombo->itemData(seriesCombo->currentIndex(), Qt::UserRole).value<void*>());
	if (!scoring)
	{
		// Scoring not loaded yet
		scoring = seriesScoringDB.getOrLoadScoring(seriesCombo->itemText(seriesCombo->currentIndex()), this);
		if (!scoring)
		{
			assert(false);
			return;
		}
		seriesCombo->setItemData(seriesCombo->currentIndex(), qVariantFromValue<void*>(scoring), Qt::UserRole);
	}
	
	suggestRaceNumber();
	updateRacesList();
	
	// Check if layouts match
	SeriesScoring::ResultMap::iterator it = scoring->beginResults();
	bool enable = (it == scoring->endResults()) || result_list->layoutMatches(it->second->list);
	statusLabel->setVisible(!enable);
	addButton->setEnabled(enable);
}
void AddToSeriesScoringDialog::addClicked()
{
	if (seriesCombo->currentIndex() == -1)
		return;
	
	int run_number = raceNumberEdit->text().toInt();
	std::set<Runner*> noOrganizersYet;
	
	added_to_scoring = static_cast<SeriesScoring*>(seriesCombo->itemData(seriesCombo->currentIndex(), Qt::UserRole).value<void*>());
	if (added_to_scoring->hasResult(run_number) && QMessageBox::question(this, APP_NAME, tr("Confirm: overwrite the results for race number %1?").arg(run_number), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
		return;
	new_result = added_to_scoring->addResult(run_number, nameEdit->text(), result_list, noOrganizersYet);
	accept();
}
void AddToSeriesScoringDialog::suggestRaceNumber()
{
	if (seriesCombo->currentIndex() == -1)
		return;
	
	SeriesScoring* scoring = static_cast<SeriesScoring*>(seriesCombo->itemData(seriesCombo->currentIndex(), Qt::UserRole).value<void*>());
	int suggestedNumber = 1;
	for (SeriesScoring::ResultMap::iterator it = scoring->beginResults(); it != scoring->endResults(); ++it)
	{
		if (suggestedNumber == it->first)
			++suggestedNumber;
		else
			break;
	}
	raceNumberEdit->setText(QString::number(suggestedNumber));
}
void AddToSeriesScoringDialog::updateRacesList()
{
	racesList->clear();
	if (seriesCombo->currentIndex() == -1)
		return;
	
	SeriesScoring* scoring = static_cast<SeriesScoring*>(seriesCombo->itemData(seriesCombo->currentIndex(), Qt::UserRole).value<void*>());
	for (SeriesScoring::ResultMap::iterator it = scoring->beginResults(); it != scoring->endResults(); ++it)
	{
		QListWidgetItem* item = new QListWidgetItem(racesList);
		item->setText(QString::number(it->first) + ". " + it->second->name);
		item->setFlags(Qt::ItemIsEnabled);
	}
}

// ### ChangeRaceNumberDialog ###

ChangeRaceNumberDialog::ChangeRaceNumberDialog(SeriesScoring* scoring, const QString& race_name, QWidget* parent): QDialog(parent), scoring(scoring)
{
	setWindowTitle(tr("Change race number of %1").arg(race_name));
	
	QListWidget* racesList = new QListWidget();
	for (SeriesScoring::ResultMap::iterator it = scoring->beginResults(); it != scoring->endResults(); ++it)
	{
		QListWidgetItem* item = new QListWidgetItem(racesList);
		item->setText(QString::number(it->first) + ". " + it->second->name);
		item->setFlags(Qt::ItemIsEnabled);
	}
	
	QLabel* label = new QLabel(tr("Change to number:"));
	edit = new QLineEdit();
	edit->setValidator(new QIntValidator(1, 999999, edit));
	QHBoxLayout* editLayout = new QHBoxLayout();
	editLayout->addWidget(label);
	editLayout->addWidget(edit);
	
	QPushButton* abortButton = new QPushButton(tr("Abort"));
	changeButton = new QPushButton(tr("Change"));
	changeButton->setEnabled(false);
	QHBoxLayout* buttonsLayout = new QHBoxLayout();
	buttonsLayout->addWidget(abortButton);
	buttonsLayout->addStretch(1);
	buttonsLayout->addWidget(changeButton);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(racesList);
	layout->addLayout(editLayout);
	layout->addLayout(buttonsLayout);
	setLayout(layout);
	
	connect(abortButton, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(changeButton, SIGNAL(clicked(bool)), this, SLOT(accept()));
	connect(edit, SIGNAL(textChanged(QString)), this, SLOT(numberChanged(QString)));
}
void ChangeRaceNumberDialog::numberChanged(QString new_text)
{
	number = new_text.toInt();
	if (number <= 0)
		changeButton->setEnabled(false);
	else
		changeButton->setEnabled(!scoring->hasResult(number));
}
