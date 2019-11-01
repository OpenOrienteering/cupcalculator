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


#include "mainWindow.h"

#include <QtWidgets>
#include <assert.h>
#include <set>

#include "global.h"
#include "csvFile.h"
#include "resultCsvImport.h"
#include "event.h"
#include "scoring.h"
#include "scoringDialog.h"
#include "seriesScoring.h"
#include "seriesScoringDialog.h"
#include "club.h"
#include "clubDialog.h"
#include "runner.h"
#include "resultList.h"
#include "util.h"
#include "presentScoring.h"

char installationKey[33];	// TODO: find a better location for this

// ### MainWindow ###

MainWindow::MainWindow(QWidget* parent)
 : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::Window)
{
	event = new Event();
	
	tabWidget = new QTabWidget;
	calculate_scoring_tab = new CalculateScoringTab(event);
	tabWidget->addTab(calculate_scoring_tab, tr("Calculate scoring"));
	tabWidget->addTab(new SeriesScoringTab(), tr("Series scorings"));
	tabWidget->addTab(new ClubDatabaseTab(this), tr("Clubs"));
	tabWidget->addTab(new RunnerDatabaseTab(this), tr("Runners"));
	tabWidget->addTab(new AboutTab(this), tr("About"));
	
	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addWidget(tabWidget);
	setLayout(mainLayout);
	
	setWindowTitle(QString("%1 %2").arg(APP_NAME).arg(APP_VERSION));
	setWindowIcon(QIcon("images/control.png"));
	
	setSizeGripEnabled(true);
	setAttribute(Qt::WA_DeleteOnClose);
}
MainWindow::~MainWindow()
{
	delete event;
}
bool MainWindow::importResultsFromCSV(CSVFile* file, bool show_import_dialog)
{
	return calculate_scoring_tab->importResultsFromCSV(file, show_import_dialog);
}
QSize MainWindow::sizeHint() const
{
    return QSize(1024, 600);
}

// ### CalculateScoringTab ###

CalculateScoringTab::CalculateScoringTab(Event* _event, QWidget* parent) : QWidget(parent)
{
	event = _event;
	
	// Scorings group
	QGroupBox* scoringsGroup = new QGroupBox(tr("1. Setup scoring(s)"));
	scoringsList = new QListWidget();
	newScoringButton = new QPushButton(QIcon("images/plus.png"), tr("New ..."));
	editScoringButton = new QPushButton(tr("Edit ..."));
	deleteScoringButton = new QPushButton(QIcon("images/minus.png"), tr("Delete"));
	
	editScoringButton->setEnabled(false);
	deleteScoringButton->setEnabled(false);
	
	QGridLayout* scoringsLayout = new QGridLayout();
	scoringsLayout->addWidget(scoringsList, 0, 0, 1, 3);
	scoringsLayout->addWidget(newScoringButton, 3, 0);
	scoringsLayout->addWidget(editScoringButton, 3, 1);
	scoringsLayout->addWidget(deleteScoringButton, 3, 2);
	scoringsGroup->setLayout(scoringsLayout);
	
	// Input group
	QGroupBox* inputGroup = new QGroupBox(tr("2. Point calculation"));
	importResultsButton = new QPushButton(QIcon("images/open.png"), tr("Import results ..."));
	closeEventButton = new QPushButton(tr("Close event"));
	closeEventButton->setEnabled(false);
	resultsTable = new ResultsTable(NULL);	// IMPORTANT NOTE: When changing anything in this list, the calculated results must be cleared! Consider this scenario: a club in this list is deleted, but still contained in the calculated scorings. Now the club can be deleted in the DB because it doesn't look in the calculated tables, but it is still referenced!
	QLabel* eventYearLabel = new QLabel(tr("Year of event:"));
	eventYearEdit = new QLineEdit();
	eventYearEdit->setValidator(new QIntValidator(eventYearEdit));
	calculateScoringsButton = new QPushButton(QIcon("images/refresh.png"), tr("Calculate scorings"));
	calculateScoringsButton->setEnabled(false);
	presentScoringButton = new QPushButton(QIcon("images/display.png"), tr("Present scoring ..."));
	presentScoringButton->setEnabled(false);
	
	QHBoxLayout* eventYearLayout = new QHBoxLayout();
	eventYearLayout->addWidget(eventYearLabel);
	eventYearLayout->addWidget(eventYearEdit);
	
	QGridLayout* inputLayout = new QGridLayout();
	inputLayout->addWidget(importResultsButton, 0, 0);
	inputLayout->addWidget(closeEventButton, 0, 1);
	inputLayout->addWidget(resultsTable, 1, 0, 1, 2);
	inputLayout->addLayout(eventYearLayout, 2, 0, 1, 2);
	inputLayout->addWidget(calculateScoringsButton, 3, 0, 1, 2);
	inputLayout->addWidget(presentScoringButton, 4, 0, 1, 2);
	inputGroup->setLayout(inputLayout);
	
	// View scorings group
	QGroupBox* viewScoringsGroup = new QGroupBox(tr("3. Export scorings"));
	QLabel* scoringLabel = new QLabel(tr("Scoring:"));
	viewScoringCombo = new QComboBox();
	scoringTable = new ResultsTable(NULL);
	addToSeriesScoringButton = new QPushButton(tr("Add to series scoring ..."));
	exportScoringButton = new QPushButton(QIcon("images/save.png"), tr("Export scoring ..."));
	
	viewScoringCombo->setEnabled(false);
	addToSeriesScoringButton->setEnabled(false);
	exportScoringButton->setEnabled(false);
	
	QGridLayout* viewScoringsLayout = new QGridLayout();
	viewScoringsLayout->addWidget(scoringLabel, 0, 0);
	viewScoringsLayout->addWidget(viewScoringCombo, 0, 1);
	viewScoringsLayout->setColumnStretch(1, 1);
	viewScoringsLayout->addWidget(scoringTable, 1, 0, 1, 2);
	viewScoringsLayout->addWidget(addToSeriesScoringButton, 2, 0, 1, 2);
	viewScoringsLayout->addWidget(exportScoringButton, 3, 0, 1, 2);
	viewScoringsGroup->setLayout(viewScoringsLayout);
	
	QHBoxLayout* mainLayout = new QHBoxLayout();
	mainLayout->addWidget(scoringsGroup);
	mainLayout->addWidget(inputGroup, 1);
	mainLayout->addWidget(viewScoringsGroup, 1);
	setLayout(mainLayout);
	
	// Set values
	for (ScoringDB::Scorings::iterator it = scoringDB.begin(); it != scoringDB.end(); ++it)
	{
		QListWidgetItem* item = new QListWidgetItem(it.key());
		item->setCheckState(Qt::Unchecked);
		item->setData(Qt::UserRole, qVariantFromValue<void*>(it.value()));
		scoringsList->addItem(item);
	}
	
	connect(newScoringButton, SIGNAL(clicked()), this, SLOT(newScoringClicked()));
	connect(editScoringButton, SIGNAL(clicked()), this, SLOT(editScoringClicked()));
	connect(deleteScoringButton, SIGNAL(clicked()), this, SLOT(removeScoringClicked()));
	
	connect(importResultsButton, SIGNAL(clicked()), this, SLOT(importResultsClicked()));
	connect(closeEventButton, SIGNAL(clicked(bool)), this, SLOT(closeEventClicked()));
	connect(calculateScoringsButton, SIGNAL(clicked()), this, SLOT(calculateScoringsClicked()));
	connect(presentScoringButton, SIGNAL(clicked()), this, SLOT(presentScoringClicked()));
	
	connect(viewScoringCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(viewScoringListChanged(int)));
	
	connect(addToSeriesScoringButton, SIGNAL(clicked()), this, SLOT(addToSeriesScoringClicked()));
	connect(exportScoringButton, SIGNAL(clicked()), this, SLOT(exportScoringsClicked()));
	
	connect(scoringsList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(currentScoringChanged(QListWidgetItem*,QListWidgetItem*)));
	connect(scoringsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(scoringChecked(QListWidgetItem*)));
	connect(scoringsList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(scoringDoubleClicked(QListWidgetItem*)));
}
void CalculateScoringTab::newScoringClicked()
{
	Scoring* new_scoring = scoringDB.createNewScoring(tr("New scoring"));
	
	ScoringDialog scoringDialog(new_scoring, this);
	scoringDialog.setWindowModality(Qt::WindowModal);
	scoringDialog.exec();
	
	event->addScoring(new_scoring);
	
	QListWidgetItem* item = new QListWidgetItem(new_scoring->getFileName());
	item->setCheckState(Qt::Checked);
	item->setData(Qt::UserRole, qVariantFromValue<void*>(new_scoring));
	scoringsList->addItem(item);
	
	updateCalculateScoringButton();
}
void CalculateScoringTab::editScoringClicked()
{
	QListWidgetItem* item = scoringsList->item(scoringsList->currentRow());
	scoringDoubleClicked(item);
}
void CalculateScoringTab::scoringDoubleClicked(QListWidgetItem* item)
{
	Scoring* scoring = reinterpret_cast<Scoring*>(item->data(Qt::UserRole).value<void*>());
	if (!scoring)
	{
		scoring = scoringDB.getOrLoadScoring(item->text(), this->window());
		if (!scoring)
			return;
		disconnect(scoringsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(scoringChecked(QListWidgetItem*)));
		item->setData(Qt::UserRole, qVariantFromValue<void*>(scoring));
		connect(scoringsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(scoringChecked(QListWidgetItem*)));
	}
	
	QString oldFilename = scoring->getFileName();
	
	ScoringDialog scoringDialog(scoring, this);
	scoringDialog.setWindowModality(Qt::WindowModal);
	scoringDialog.exec();
	
	if (scoring->getFileName() != oldFilename)
	{
		disconnect(scoringsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(scoringChecked(QListWidgetItem*)));
		item->setText(scoring->getFileName());
		connect(scoringsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(scoringChecked(QListWidgetItem*)));
	}
}
void CalculateScoringTab::removeScoringClicked()
{
	QListWidgetItem* item = scoringsList->item(scoringsList->currentRow());
	
	if (QMessageBox::question(this, APP_NAME, tr("Confirm: delete the scoring %1?").arg(item->text()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
		return;
	
	if (item->checkState() == Qt::Checked)
	{
		Scoring* scoring = reinterpret_cast<Scoring*>(item->data(Qt::UserRole).value<void*>());
		if (scoring)
		{
			event->removeScoring(scoring);
			updateCalculateScoringButton();
		}
	}
	scoringDB.deleteScoring(item->text());
	delete item;
}
void CalculateScoringTab::currentScoringChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
	bool enable = current != NULL;
	editScoringButton->setEnabled(enable);
	deleteScoringButton->setEnabled(enable);
}
void CalculateScoringTab::scoringChecked(QListWidgetItem* item)
{
	if (item->checkState() == Qt::Checked)
	{
		// Make sure the scoring is loaded
		Scoring* scoring = reinterpret_cast<Scoring*>(item->data(Qt::UserRole).value<void*>());
		if (!scoring)
		{
			scoring = scoringDB.getOrLoadScoring(item->text(), this->window());
			disconnect(scoringsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(scoringChecked(QListWidgetItem*)));
			if (scoring)
				item->setData(Qt::UserRole, qVariantFromValue<void*>(scoring));
			else
				item->setCheckState(Qt::Unchecked);
			connect(scoringsList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(scoringChecked(QListWidgetItem*)));
			if (!scoring)
				return;
		}
		
		event->addScoring(scoring);
	}
	else
	{
		Scoring* scoring = reinterpret_cast<Scoring*>(item->data(Qt::UserRole).value<void*>());
		event->removeScoring(scoring);
	}
	
	updateCalculateScoringButton();
}
void CalculateScoringTab::updateCalculateScoringButton()
{
	bool enable = event->getNumScorings() > 0 && event->getResultList() && event->getResultList()->rowCount() > 0;
	calculateScoringsButton->setEnabled(enable);
	presentScoringButton->setEnabled(enable);
	
	closeEventButton->setEnabled(event->getResultList());	// TODO: rename method to reflect this also
}
void CalculateScoringTab::importResultsClicked()
{
	QString path = QFileDialog::getOpenFileName(this, tr("Import results ..."), QString(), tr("Sportsoftware CSV files (*.csv);;All files (*.*)"));
	path = QFileInfo(path).canonicalFilePath();
	
	if (path.isEmpty())
		return;
	
	CSVFile file(path);
	if (!file.open(true))
	{
		QMessageBox::warning(this, APP_NAME, tr("Could not open file:\n") + path);
		return;
	}
	
	if (importResultsFromCSV(&file))
	{
		eventYearEdit->setText(QString::number(QFileInfo(path).lastModified().date().year()));
		viewScoringCombo->clear();
		event->clearScoringLists();
	}
	
	updateCalculateScoringButton();
}
void CalculateScoringTab::closeEventClicked()
{
	resultsTable->setModel(NULL);
	event->setResultList(NULL);
	viewScoringCombo->clear();
	event->clearScoringLists();
	
	updateCalculateScoringButton();
}
void CalculateScoringTab::calculateScoringsClicked()
{
	if (event->getNumScorings() <= 0)
	{
		QMessageBox::warning(this, APP_NAME, tr("Please set up a scoring first!"));
		return;
	}
	
	int event_year = eventYearEdit->text().toInt();
	event->calculateScorings(event_year);
	
	viewScoringCombo->clear();
	for (int i = 0; i < event->getNumScoringLists(); ++i)
	{
		ResultList* scoringList = event->getScoringList(i);
		viewScoringCombo->addItem(scoringList->getTitle(), qVariantFromValue<void*>(scoringList));
	}
	
	viewScoringCombo->setEnabled(event->getNumScoringLists() > 0);
}
void CalculateScoringTab::presentScoringClicked()
{
	if (event->getNumScorings() <= 0)
	{
		QMessageBox::warning(this, APP_NAME, tr("Please set up a scoring first!"));
		return;
	}
	
	int event_year = eventYearEdit->text().toInt();
	
	PresentScoringDialog presentDialog(event, event_year, event->getResultList(), this);
	presentDialog.setWindowModality(Qt::WindowModal);
	presentDialog.exec();
}
void CalculateScoringTab::addToSeriesScoringClicked()
{
	if (seriesScoringDB.size() == 0)
	{
		QMessageBox::warning(this, APP_NAME, tr("Please create a series scoring in the respective tab first!"));
		return;
	}
	
	int index = viewScoringCombo->currentIndex();
	ResultList* scoringList = reinterpret_cast<ResultList*>(viewScoringCombo->itemData(index, Qt::UserRole).value<void*>());
	
	AddToSeriesScoringDialog addToSeriesScoringDialog(scoringList, this);
	addToSeriesScoringDialog.setWindowModality(Qt::WindowModal);
	
	//if (addToSeriesScoringDialog.exec() == QDialog::Accepted)
	addToSeriesScoringDialog.exec();
	if (addToSeriesScoringDialog.getNewSeriesRaceResult() != NULL)
	{
		SeriesScoringDialog scoringDialog(addToSeriesScoringDialog.getAddedToScoring(), this);
		scoringDialog.setWindowModality(Qt::WindowModal);
		scoringDialog.jumpToRaceResult(addToSeriesScoringDialog.getNewSeriesRaceResult());
		scoringDialog.exec();
	}
}
void CalculateScoringTab::exportScoringsClicked()
{
	int index = viewScoringCombo->currentIndex();
	ResultList* scoringList = reinterpret_cast<ResultList*>(viewScoringCombo->itemData(index, Qt::UserRole).value<void*>());
	
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
	scoringList->exportToCSV(&file);
	file.close();
	
	QMessageBox::information(this, APP_NAME, tr("Export successful."));
}
void CalculateScoringTab::viewScoringListChanged(int index)
{
	if (index < 0)
	{
		scoringTable->setModel(NULL);
		exportScoringButton->setEnabled(false);
		addToSeriesScoringButton->setEnabled(false);
		return;
	}
	
	ResultList* scoringList = reinterpret_cast<ResultList*>(viewScoringCombo->itemData(index, Qt::UserRole).value<void*>());
	scoringTable->setModel(scoringList);
	exportScoringButton->setEnabled(true);
	addToSeriesScoringButton->setEnabled(true);
}

bool CalculateScoringTab::importResultsFromCSV(CSVFile* file, bool show_import_dialog)
{
	// Show import dialog
	CSVImportDialog importDialog(file, this);
	QString maleString = "M";
	if (show_import_dialog)
	{
		importDialog.setWindowModality(Qt::WindowModal);
		importDialog.exec();
		if (importDialog.result() == QDialog::Rejected)
			return false;
		maleString = importDialog.maleString;
	}
	
	ResultList* results = new ResultList(tr("Race results"), 0);
	int num_runners = (int)importDialog.colFirstName->size();
	results->addRaceTimesColumns(num_runners);
	Runner** runners = new Runner*[num_runners];
	int* runner_times = NULL;
	if (num_runners > 1)
		runner_times = new int[num_runners];
	
	int baseDate = (QDate::currentDate().year() / 100) * 100;
	int lastBaseDate = baseDate - 100;
	int baseDateChange = QDate::currentDate().year() % 100;
	
	ProblemWidget* problemWidget = new ProblemWidget();
	std::set<Club*> addedClubs;
	std::set<Runner*> addedRunners;
	
	// Read lines
	while (file->nextLine())
	{
		// Find or create category
		QString categoryName = file->getValue(importDialog.colCategory->at(0));
		Category* category = event->findCategory(categoryName);
		
		if (!category)
		{
			category = new Category();
			category->name = categoryName;
			event->addCategory(category);
		}
		
		// Find or create club
		QString clubName = file->getValue(importDialog.colClubPart1->at(0));
		if (importDialog.colClubPart2)
		{
			if (!clubName.isEmpty())
				clubName += " ";
			clubName += file->getValue(importDialog.colClubPart2->at(0));
		}
		
		if (clubName.isEmpty() && importDialog.colClubPart1Backup)
		{
			clubName = file->getValue(importDialog.colClubPart1Backup->at(0));
			if (importDialog.colClubPart2Backup)
			{
				if (!clubName.isEmpty())
					clubName += " ";
				clubName += file->getValue(importDialog.colClubPart2Backup->at(0));
			}
		}
		
		Club* club = NULL;
		if (clubName.isEmpty())
		{
			if (importDialog.skip_empty_club_rows)
				continue;
		}
		else
		{
			club = clubDB.findClub(clubName);
			if (!club)
			{
				club = new Club(clubName);
				
				// Look for typos
				std::vector<Club*> similarClubs;
				clubDB.findSimilarClubs(clubName, similarClubs);
				for (int i = (int)similarClubs.size() - 1; i >= 0; --i)
				{
					// Don't offer to merge clubs which come both from the imported results, could lead to problematic situations
					if (addedClubs.find(similarClubs[i]) != addedClubs.end())
						similarClubs.erase(similarClubs.begin() + i);
				}
				if (similarClubs.size())
					problemWidget->addProblem(new SimilarClubProblem(club, similarClubs, results));
				
				clubDB.addClub(club);
				addedClubs.insert(club);
			}
		}
		
		// Find or create runner(s)
		for (int i = 0; i < num_runners; ++i)
		{
			QString firstName = file->getValue(importDialog.colFirstName->at(i));
			QString lastName = file->getValue(importDialog.colLastName->at(i));
			
			// Special case: The Sportsoftware sometimes exports numbers as last names for non-existent runners when there are up to 3 runners in teams in a race, but some categories have fewer
			QString name = firstName + " " + lastName;
			bool no_runner = name.trimmed().isEmpty();
			if (!no_runner)
				name.toInt(&no_runner);
			if (no_runner)
			{
				runners[i] = NULL;
				if (num_runners > 1)
					runner_times[i] = -1;
				continue;
			}
			
			int year;
			if (importDialog.colYear)
			{
				year = file->getValue(importDialog.colYear->at(i)).toInt();
				if (year <= baseDateChange)
					year += baseDate;
				else if (year < 100)
					year += lastBaseDate;
			}
			else
				year = 0;
			QString isMale_Str = file->getValue(importDialog.colGender->at(i));
			bool isMale = (isMale_Str == maleString);
			
			Runner runner_data;
			runner_data.setFirstName(firstName);
			runner_data.setLastName(lastName);
			if (club)
				runner_data.addClub(club);
			runner_data.setIsMale(isMale);
			runner_data.setYear(year);
			
			runners[i] = runnerDB.findRunner(runner_data);
			if (runners[i])
			{
				if (club)
					runners[i]->addClub(club);
			}
			else
			{
				runners[i] = new Runner(runner_data);
				
				// Look for typos
				std::set<Runner*> similarRunners;
				runnerDB.findSimilarRunners(runners[i], similarRunners);
				for (std::set<Runner*>::iterator it = addedRunners.begin(); it != addedRunners.end(); ++it)
				{
					// Don't offer to merge runners who both come from the imported results, could lead to problematic situations
					std::set<Runner*>::iterator s_it = similarRunners.find(*it);
					if (s_it != similarRunners.end())
						similarRunners.erase(s_it);
				}
				if (similarRunners.size())
				{
					std::vector<Runner*> similarRunnersVector;
					for (std::set<Runner*>::iterator it = similarRunners.begin(); it != similarRunners.end(); ++it)
						similarRunnersVector.push_back(*it);
					problemWidget->addProblem(new SimilarRunnerProblem(runners[i], similarRunnersVector, results));
				}
				
				runnerDB.addRunner(runners[i]);
				addedRunners.insert(runners[i]);
			}
			
			if (num_runners > 1)
			{
				QString time_text = file->getValue(importDialog.colTime->at(i + 1));
				runner_times[i] = parseTime(time_text);
				if (runner_times[i] < 0)
					runner_times[i] = getCorrectedTime(time_text);
			}
		}

		// Read time
		QString time_text = file->getValue(importDialog.colTime->at(0));
		int raceTime = parseTime(time_text);
		if (raceTime < 0)
			raceTime = getCorrectedTime(time_text);
		
		// Read state
		ResultList::ResultType resultType;
		if (importDialog.colState)
		{
			int sportsoftwareResultId = file->getValue(importDialog.colState->at(0)).toInt();
			
			switch (sportsoftwareResultId)
			{
			case 0:	resultType = ResultList::ResultOk;				break;
			case 1:	resultType = ResultList::ResultDidNotStart;		break;
			case 2:	resultType = ResultList::ResultDidNotFinish;	break;
			case 3:	resultType = ResultList::ResultMispunch;		break;
			case 4:	resultType = ResultList::ResultDisqualified;	break;
			case 5:	resultType = ResultList::ResultOvertime;		break;
			}
		}
		else
			resultType = ResultList::ResultOk;
				
		// Add result list entry
		int row = results->addRow();
		results->setData(row, results->getCategoryColumn(), qVariantFromValue<void*>(category));
		if (importDialog.colNotClassified && file->getValue(importDialog.colNotClassified->at(0)).compare("x", Qt::CaseInsensitive) == 0)
			results->setData(row, results->getRankColumn(), QVariant());	// noncompetitive
		else
			results->setData(row, results->getRankColumn(), QVariant(-1));
		if (num_runners == 1)
			results->setData(row, results->getFirstRunnerColumn(), qVariantFromValue<void*>(runners[0]));
		else
		{
			for (int i = 0; i < num_runners; ++i)
			{
				results->setData(row, results->getFirstRunnerColumn() + 2*i, qVariantFromValue<void*>(runners[i]));
				results->setData(row, results->getFirstRunnerColumn() + 2*i + 1, runner_times[i] >= 0 ? runner_times[i] : QVariant());
			}
		}
		results->setData(row, results->getClubColumn(), qVariantFromValue<void*>(club));
		results->setData(row, results->getStatusColumn(), QVariant(resultType));
		results->setData(row, results->getTimeColumn(), QVariant(raceTime));
	}
	
	delete[] runner_times;
	delete[] runners;
	
	if (problemWidget->problemCount() > 0)
		problemWidget->showAsDialog(tr("Problems while importing results"), window());
	else
		delete problemWidget;
	
	// Check for errors
	if (file->getError() != CSVFile::NoError)
	{
		if (file->getError() == CSVFile::InvalidLine)
			QMessageBox::warning(this, APP_NAME, tr("Error while reading file:\n") + file->getPath() + tr("\n\nEncountered an incorrectly formatted line."));
		else
			QMessageBox::warning(this, APP_NAME, tr("Error while reading file:\n") + file->getPath() + tr("\n\nUnknown error."));
		
		delete results;
		return false;
	}
	
	// Calculate ranks
	results->calculateRanks(results->getTimeColumn(), results->getStatusColumn(), true);
	
	resultsTable->setModel(results);
	event->setResultList(results);
	
	file->close();
	return true;
}
int CalculateScoringTab::getCorrectedTime(const QString& time_text)
{
	int result;
	while (true)
	{
		bool ok;
		QString customTime = QInputDialog::getText(this, APP_NAME, tr("Time formatting not supported:\n") + time_text + tr("\n\nPlease enter the time in HH:MM:SS, MMM:SS, HH:MM:SS,T or MMM:SS,T format:"), QLineEdit::Normal, time_text, &ok);
		if (ok)
		{
			result = parseTime(customTime);
			if (result < 0)
				QMessageBox::warning(this, APP_NAME, tr("Error: wrong formatting."));	// TODO: better error report mechanism (problem dialog)
			else
				return result;
		}
		else
			return 0;
	}
}

// ### ClubDatabaseTab ###

ClubDatabaseTab::ClubDatabaseTab(MainWindow* mainWindow, QWidget* parent): QWidget(parent)
{
	this->mainWindow = mainWindow;
	
	clubList = new ClubTable();
	
	editButton = new QPushButton(tr("Edit ..."));
	editButton->setEnabled(false);
	addButton = new QPushButton(QIcon("images/plus.png"), "");
	removeButton = new QPushButton(QIcon("images/minus.png"), "");
	removeButton->setEnabled(false);
	mergeButton = new QPushButton(tr("Merge ..."));
	mergeButton->setEnabled(false);
	
	QGridLayout* mainLayout = new QGridLayout();
	mainLayout->addWidget(clubList, 0, 0, 1, 5);
	mainLayout->addWidget(addButton, 1, 0);
	mainLayout->addWidget(removeButton, 1, 1);
	mainLayout->setColumnStretch(2, 1);
	mainLayout->addWidget(mergeButton, 1, 3);
	mainLayout->addWidget(editButton, 1, 4);

	setLayout(mainLayout);
	
	connect(editButton, SIGNAL(clicked(bool)), this, SLOT(editClicked()));
	connect(addButton, SIGNAL(clicked(bool)), this, SLOT(addClicked()));
	connect(removeButton, SIGNAL(clicked(bool)), this, SLOT(removeClicked()));
	connect(mergeButton, SIGNAL(clicked(bool)), this, SLOT(mergeClicked()));
	
	connect(clubList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(updateActions()));
	connect(clubList, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editClicked()));
}
void ClubDatabaseTab::editClicked()
{
	QModelIndex index = clubList->selectionModel()->currentIndex();
	if (!index.isValid())
		return;
	QModelIndex sourceIndex = clubList->getSortedModel()->mapToSource(index);
	
	Club* club = clubDB.getItem(sourceIndex);
	
	ClubDialog clubDialog(club, this);
	clubDialog.setWindowModality(Qt::WindowModal);
	clubDialog.exec();
	clubDB.clubChanged(sourceIndex);
}
void ClubDatabaseTab::addClicked()
{
	QModelIndex index = clubList->selectionModel()->currentIndex();
	QAbstractItemModel* model = &clubDB;
	
	int row;
	if (index.isValid())
		row = index.row() + 1;
	else
		row = model->rowCount();
	
	if (!model->insertRow(row))
		return;
	
	clubList->selectionModel()->setCurrentIndex(clubList->getSortedModel()->mapFromSource(model->index(row, 0)), QItemSelectionModel::ClearAndSelect);
	updateActions();
	
	// Show dialog
	QModelIndex new_index = model->index(row, 0);
	Club* club = clubDB.getItem(new_index);
	
	ClubDialog clubDialog(club, this);
	clubDialog.setWindowModality(Qt::WindowModal);
	clubDialog.exec();
	clubDB.clubChanged(new_index);
}
void ClubDatabaseTab::removeClicked()
{
	QAbstractItemModel* model = &clubDB;
	
	// Check if selected clubs can be deleted
	ResultList* results = mainWindow->getEvent()->getResultList();
	std::set< int > checkedRows;
	QModelIndexList indexes = clubList->selectionModel()->selectedIndexes();
	QModelIndex index;
	foreach (index, indexes) {
		if (checkedRows.insert(index.row()).second == true)
		{
			Club* club = clubDB.getItem(clubList->getSortedModel()->mapToSource(index));
			
			// Look for club in the club list of every runner
			if (runnerDB.findRunnersInClub(club).size() > 0)
			{
				// TODO: offer to do the suggested
				QMessageBox::warning(this, APP_NAME, tr("A club you want to delete has still members. You must remove the members first!"));
				return;
			}
			
			// Look for club in result list
			if (results && results->findInColumn(results->getClubColumn(), qVariantFromValue<void*>(club)) >= 0)
			{
				// TODO: Could offer to do the suggested things
				QMessageBox::warning(this, APP_NAME, tr("The club %1 you want to delete is in the result list of the current event. Either close the event or delete the occurences of the club from the event first!").arg(club->getName()));
				return;
			}
		}
	}
	
	if (QMessageBox::question(this, APP_NAME, tr("Confirm: delete the selected clubs?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
		return;
	
	int sel_row = clubList->selectionModel()->currentIndex().row();
	int sel_column = clubList->selectionModel()->currentIndex().column();
	
	for (std::set< int >::reverse_iterator rit = checkedRows.rbegin(); rit != checkedRows.rend(); ++rit)
		clubList->getSortedModel()->removeRow(*rit);
	
	if (sel_row < clubList->model()->rowCount())
		clubList->selectionModel()->setCurrentIndex(model->index(sel_row, sel_column), QItemSelectionModel::ClearAndSelect);
	else if (clubList->model()->rowCount() > 0)
		clubList->selectionModel()->setCurrentIndex(model->index(clubList->model()->rowCount() - 1, sel_column), QItemSelectionModel::ClearAndSelect);
	else
		updateActions();
}
void ClubDatabaseTab::mergeClicked()
{
	std::set<int> checkedRows;
	std::vector<Club*> clubs;
	QModelIndexList indexes = clubList->selectionModel()->selectedIndexes();
	QModelIndex index;
	foreach (index, indexes) {
		if (checkedRows.insert(index.row()).second == true)
		{
			Club* club = clubDB.getItem(clubList->getSortedModel()->mapToSource(index));
			clubs.push_back(club);
		}
	}
	
	QMessageBox box(window());
	box.setWindowTitle(tr("Merge clubs"));
	box.setText(tr("Merge to which club?"));
	QString abortText = tr("Abort");
	box.addButton(abortText, QMessageBox::RejectRole);
	std::map<QAbstractButton*, Club*> buttonToClub;
	for (int i = 0; i < (int)clubs.size(); ++i)
	{
		QPushButton* button = new QPushButton(clubs[i]->getName());
		box.addButton(button, QMessageBox::YesRole);
		buttonToClub.insert(std::make_pair(button, clubs[i]));
	}
	box.exec();
	
	if (box.clickedButton()->text() == abortText)
		return;
	
	Club* destClub = buttonToClub[box.clickedButton()];
	ResultList* results = mainWindow->getEvent()->getResultList();
	
	for (int k = 0; k < (int)clubs.size(); ++k)
	{
		if (destClub != clubs[k])
		{
			if (results)
				results->changeInColumn(results->getClubColumn(), qVariantFromValue<void*>(clubs[k]), qVariantFromValue<void*>(destClub));
			clubDB.mergeClubs(clubs[k], destClub);
		}
	}
}
void ClubDatabaseTab::updateActions()
{
	bool hasSelection = !clubList->selectionModel()->selection().isEmpty();
	removeButton->setEnabled(hasSelection);
	
	bool hasCurrent = clubList->selectionModel()->currentIndex().isValid();
	editButton->setEnabled(hasCurrent);
	
	int num_selected_rows = 0;
	std::set< int > checkedRows;
	QModelIndexList indexes = clubList->selectionModel()->selectedIndexes();
	QModelIndex index;
	foreach (index, indexes) {
		if (checkedRows.insert(index.row()).second == true)
			++num_selected_rows;
	}
	mergeButton->setEnabled(num_selected_rows >= 2 && num_selected_rows < 10);
}

// ### RunnerDatabaseTab ###

RunnerDatabaseTab::RunnerDatabaseTab(MainWindow* mainWindow, QWidget* parent): QWidget(parent)
{
	this->mainWindow = mainWindow;
	
	// Runner list
	runnerList = new RunnerTable();
	
	addButton = new QPushButton(QIcon("images/plus.png"), "");
	removeButton = new QPushButton(QIcon("images/minus.png"), "");
	mergeButton = new QPushButton(tr("Merge ..."));
	
	QGridLayout* runnerLayout = new QGridLayout();
	runnerLayout->addWidget(runnerList, 0, 0, 1, 4);
	runnerLayout->addWidget(addButton, 1, 0);
	runnerLayout->addWidget(removeButton, 1, 1);
	runnerLayout->addWidget(mergeButton, 1, 3);
	runnerLayout->setColumnStretch(2, 1);
	
	// Club list
	QLabel* clubListLabel = new QLabel(tr("Clubs of the selected runner:"));
	
	clubView = new QListView();
	sortedClubModel = new QSortFilterProxyModel(this);
	sortedClubModel->setDynamicSortFilter(true);
	sortedClubModel->setSourceModel(&clubDB);
	sortedClubModel->sort(0);
	clubView->setModel(sortedClubModel);
	clubView->setUniformItemSizes(true);
	clubView->setSelectionMode(QAbstractItemView::MultiSelection);
	
	QVBoxLayout* clubLayout = new QVBoxLayout();
	clubLayout->addWidget(clubListLabel);
	clubLayout->addWidget(clubView);
	
	QHBoxLayout* mainLayout = new QHBoxLayout();
	mainLayout->addLayout(runnerLayout, 1);
	mainLayout->addLayout(clubLayout);
	setLayout(mainLayout);
	
	connect(addButton, SIGNAL(clicked(bool)), this, SLOT(addClicked()));
	connect(removeButton, SIGNAL(clicked(bool)), this, SLOT(removeClicked()));
	connect(mergeButton, SIGNAL(clicked(bool)), this, SLOT(mergeClicked()));
	
	connect(runnerList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(updateActions()));
	connect(clubView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(clubSelectionChanged()));
	
	updateActions();
}
RunnerDatabaseTab::~RunnerDatabaseTab()
{
	clubView->setModel(NULL);
}

void RunnerDatabaseTab::addClicked()
{
	QModelIndex index = runnerList->selectionModel()->currentIndex();
	QAbstractItemModel* model = &runnerDB;
	
	int row;
	/*if (index.isValid())
		row = index.row() + 1;
	else*/
		row = model->rowCount();
	
	if (!model->insertRow(row))
		return;
	
	runnerList->selectionModel()->setCurrentIndex(runnerList->getSortedModel()->mapFromSource(model->index(row, 0)), QItemSelectionModel::ClearAndSelect);
	runnerList->setFocus();
}
void RunnerDatabaseTab::removeClicked()
{
	QAbstractItemModel* model = &runnerDB;
	
	// Check if selected runners can be deleted
	ResultList* results = mainWindow->getEvent()->getResultList();
	std::set< int > checkedRows;
	QModelIndexList indexes = runnerList->selectionModel()->selectedIndexes();
	QModelIndex index;
	foreach (index, indexes) {
		if (checkedRows.insert(index.row()).second == true)
		{
			Runner* runner = runnerDB.getItem(runnerList->getSortedModel()->mapToSource(index));
			if (results)
			{
				for (int i = results->getFirstRunnerColumn(); i <= results->getLastRunnerColumn(); ++i)
				{
					if (results->findInColumn(i, qVariantFromValue<void*>(runner)) >= 0)
					{
						// TODO: Could offer to do the suggested things
						QMessageBox::warning(this, APP_NAME, tr("The runner %1 %2 you want to delete is in the result list of the current event. Either close the event or delete the runner from the event first!").arg(runner->getFirstName()).arg(runner->getLastName()));
						return;
					}
				}
			}
			if (seriesScoringDB.openScoringReferencesRunner(runner))
			{
				// TODO: Could offer to do the suggested things
				QMessageBox::warning(this, APP_NAME, tr("The runner %1 %2 you want to delete is still referenced by at least one open series scoring. Please remove all references first!").arg(runner->getFirstName()).arg(runner->getLastName()));
				return;
			}
		}
	}
	
	if (QMessageBox::question(this, APP_NAME, tr("Confirm: delete the selected runners?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
		return;
	
	int sel_row = runnerList->selectionModel()->currentIndex().row();
	int sel_column = runnerList->selectionModel()->currentIndex().column();
	
	for (std::set< int >::reverse_iterator rit = checkedRows.rbegin(); rit != checkedRows.rend(); ++rit)
		runnerList->getSortedModel()->removeRow(*rit);
	
	if (sel_row < runnerList->model()->rowCount())
		runnerList->selectionModel()->setCurrentIndex(model->index(sel_row, sel_column), QItemSelectionModel::ClearAndSelect);
	else if (runnerList->model()->rowCount() > 0)
		runnerList->selectionModel()->setCurrentIndex(model->index(runnerList->model()->rowCount() - 1, sel_column), QItemSelectionModel::ClearAndSelect);
	else
		updateActions();
}
void RunnerDatabaseTab::mergeClicked()
{
	std::set<int> checkedRows;
	std::vector<Runner*> runners;
	QModelIndexList indexes = runnerList->selectionModel()->selectedIndexes();
	QModelIndex index;
	foreach (index, indexes) {
		if (checkedRows.insert(index.row()).second == true)
		{
			Runner* runner = runnerDB.getItem(runnerList->getSortedModel()->mapToSource(index));
			runners.push_back(runner);
		}
	}
	
	QMessageBox box(window());
	box.setWindowTitle(tr("Merge runners"));
	box.setText(tr("Merge to which runner?"));
	QString abortText = tr("Abort");
	box.addButton(abortText, QMessageBox::RejectRole);
	std::map<QAbstractButton*, Runner*> buttonToRunner;
	for (int i = 0; i < (int)runners.size(); ++i)
	{
		QPushButton* button = new QPushButton(runners[i]->getShortDesc());
		box.addButton(button, QMessageBox::YesRole);
		buttonToRunner.insert(std::make_pair(button, runners[i]));
	}
	box.exec();
	
	if (box.clickedButton()->text() == abortText)
		return;
	
	Runner* destRunner = buttonToRunner[box.clickedButton()];
	ResultList* results = mainWindow->getEvent()->getResultList();
	
	for (int k = 0; k < (int)runners.size(); ++k)
	{
		if (destRunner != runners[k])
		{
			if (results)
				for (int i = results->getFirstRunnerColumn(); i <= results->getLastRunnerColumn(); ++i)
					results->changeInColumn(i, qVariantFromValue<void*>(runners[k]), qVariantFromValue<void*>(destRunner));
			runnerDB.mergeRunners(runners[k], destRunner);
		}
	}
}
void RunnerDatabaseTab::updateActions()
{
	bool hasSelection = !runnerList->selectionModel()->selection().isEmpty();
	removeButton->setEnabled(hasSelection);
	
	bool singleRowSelected = hasSelection;
	if (singleRowSelected)
	{
		int selectedRow = runnerList->selectionModel()->currentIndex().row();
		QModelIndexList clubList = runnerList->selectionModel()->selectedIndexes();
		QModelIndex index;
		foreach (index, clubList) {
			if (index.row() != selectedRow)
			{
				singleRowSelected = false;
				break;
			}
		}
	}
	
	mergeButton->setEnabled(hasSelection && !singleRowSelected);
	
	clubView->setEnabled(singleRowSelected);
	if (singleRowSelected)
	{
		// Select all clubs of the current runner
		QItemSelectionModel* selection = clubView->selectionModel();
		Runner* runner = runnerDB.getItem(runnerList->getSortedModel()->mapToSource(runnerList->getSortedModel()->index(runnerList->selectionModel()->currentIndex().row(), 0)));
		
		disconnect(clubView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(clubSelectionChanged()));
		selection->clear();
		int numClubs = clubDB.rowCount();
		for (int i = 0; i < numClubs; ++i)
		{
			QModelIndex index = clubDB.index(i, 0);
			Club* club = clubDB.getItem(index);
			if (runner->isInClub(club))
				selection->select(sortedClubModel->mapFromSource(index), QItemSelectionModel::Select);
		}
		connect(clubView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(clubSelectionChanged()));
	}
}
void RunnerDatabaseTab::clubSelectionChanged()
{
	// Change clubs of the current runner to selection
	if (!runnerList->selectionModel()->currentIndex().isValid())
		return;
	
	QItemSelectionModel* selection = clubView->selectionModel();
	QModelIndex sourceIndex = runnerList->getSortedModel()->mapToSource(runnerList->selectionModel()->currentIndex());
	Runner* runner = runnerDB.getItem(sourceIndex);
	
	runner->clearClubs();
	QModelIndexList clubList = selection->selectedIndexes();
	QModelIndex index;
	foreach (index, clubList) {
		runner->addClub(clubDB.getItem(sortedClubModel->mapToSource(index)));
	}
	
	runnerDB.clubsChangedInRow(sourceIndex.row());
}

// ### SeriesScoringTab ###

SeriesScoringTab::SeriesScoringTab(QWidget* parent) : QWidget(parent)
{
	// Scorings group
	seriesList = new QListWidget();
	QPushButton* newButton = new QPushButton(QIcon("images/plus.png"), "");
	deleteButton = new QPushButton(QIcon("images/minus.png"), "");
	editButton = new QPushButton(tr("Edit ..."));
	
	QGridLayout* layout = new QGridLayout();
	layout->addWidget(seriesList, 0, 0, 1, 4);
	layout->addWidget(newButton, 3, 0);
	layout->addWidget(deleteButton, 3, 1);
	layout->setColumnStretch(2, 1);
	layout->addWidget(editButton, 3, 3);

	setLayout(layout);
	
	editButton->setEnabled(false);
	deleteButton->setEnabled(false);
	
	// Set values
	for (SeriesScoringDB::Scorings::iterator it = seriesScoringDB.begin(); it != seriesScoringDB.end(); ++it)
	{
		QListWidgetItem* item = new QListWidgetItem(it.key());
		item->setData(Qt::UserRole, qVariantFromValue<void*>(it.value()));
		seriesList->addItem(item);
	}
	
	connect(newButton, SIGNAL(clicked()), this, SLOT(addClicked()));
	connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
	connect(editButton, SIGNAL(clicked()), this, SLOT(editClicked()));
	
	connect(seriesList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(currentSeriesChanged(QListWidgetItem*,QListWidgetItem*)));
	connect(seriesList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(seriesDoubleClicked(QListWidgetItem*)));
}

void SeriesScoringTab::addClicked()
{
	SeriesScoring* new_scoring = seriesScoringDB.createNewScoring(tr("New series scoring"));
	
	SeriesScoringDialog scoringDialog(new_scoring, this);
	scoringDialog.setWindowModality(Qt::WindowModal);
	scoringDialog.exec();
	
	QListWidgetItem* item = new QListWidgetItem(new_scoring->getFileName());
	item->setData(Qt::UserRole, qVariantFromValue<void*>(new_scoring));
	seriesList->addItem(item);
}
void SeriesScoringTab::deleteClicked()
{
	QListWidgetItem* item = seriesList->item(seriesList->currentRow());
	
	if (QMessageBox::question(this, APP_NAME, tr("Confirm: delete the series scoring %1?").arg(item->text()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
		return;
		
	seriesScoringDB.deleteScoring(item->text());
	delete item;
}
void SeriesScoringTab::editClicked()
{
	QListWidgetItem* item = seriesList->item(seriesList->currentRow());
	seriesDoubleClicked(item);
}

void SeriesScoringTab::currentSeriesChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
	bool enable = current != NULL;
	editButton->setEnabled(enable);
	deleteButton->setEnabled(enable);
}
void SeriesScoringTab::seriesDoubleClicked(QListWidgetItem* item)
{
	SeriesScoring* scoring = reinterpret_cast<SeriesScoring*>(item->data(Qt::UserRole).value<void*>());
	if (!scoring)
	{
		scoring = seriesScoringDB.getOrLoadScoring(item->text(), window());
		if (!scoring)
			return;
		item->setData(Qt::UserRole, qVariantFromValue<void*>(scoring));
	}
	
	QString oldFilename = scoring->getFileName();
	
	SeriesScoringDialog scoringDialog(scoring, this);
	scoringDialog.setWindowModality(Qt::WindowModal);
	scoringDialog.exec();
	
	if (scoring->getFileName() != oldFilename)
		item->setText(scoring->getFileName());
}

// ### AboutTab ###

AboutTab::AboutTab(MainWindow* mainWindow, QWidget* parent): QWidget(parent), mainWindow(mainWindow)
{
	QLabel* programLabel = new QLabel(QString("<b>%1 %2</b>").arg(APP_NAME).arg(APP_VERSION));
	programLabel->setAlignment(Qt::AlignHCenter);
	QFont hugeFont = programLabel->font();
	hugeFont.setPointSize(20);
	programLabel->setFont(hugeFont);
	
	QLabel* aboutLabel = new QLabel(tr("Copyright (C) 2011, 2012, 2015  Thomas Sch&ouml;ps<br>"
									   "This program comes with ABSOLUTELY NO WARRANTY;<br>"
									   "This is free software, and you are welcome to redistribute it<br>"
									   "under certain conditions; see the file COPYING for details."));
	aboutLabel->setAlignment(Qt::AlignHCenter);
	QFont bigFont = aboutLabel->font();
	bigFont.setPointSize(12);
	aboutLabel->setFont(bigFont);
	
	QLabel* imageLabel = new QLabel(tr("This program is part of the OpenOrienteering project"));
	imageLabel->setAlignment(Qt::AlignHCenter);
	QFont mediumFont = imageLabel->font();
	mediumFont.setPointSize(12);
	imageLabel->setFont(mediumFont);
	
	QLabel* aboutImage = new QLabel("<a href=\"http://www.openorienteering.org\"><img src=\"images/open_orienteering.png\"/></a>");
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addStretch(1);
	layout->addWidget(programLabel, 0, Qt::AlignHCenter);
	layout->addWidget(aboutLabel, 0, Qt::AlignHCenter);
	layout->addSpacing(32);
	layout->addWidget(imageLabel, 0, Qt::AlignHCenter);
	layout->addWidget(aboutImage, 0, Qt::AlignHCenter);
	layout->addStretch(1);
	setLayout(layout);
	
	connect(aboutImage, SIGNAL(linkActivated(QString)), this, SLOT(linkClicked(QString)));
}
AboutTab::~AboutTab()
{
}
void AboutTab::linkClicked(QString link)
{
	QDesktopServices::openUrl(link);
}

// ### SimilarClubProblem ###

SimilarClubProblem::SimilarClubProblem(Club* club, std::vector< Club* >& similarClubs, ResultList* results)
 : club(club), similarClubs(similarClubs), results(results)
{
	QString similarClubsStr = "";
	int size = similarClubs.size();
	for (int i = 0; i < size - 1; ++i)
	{
		similarClubsStr += similarClubs[i]->getName();
		if (i < size - 2)
			similarClubsStr += ", ";
	}
	if (size > 1)
		similarClubsStr += " " + tr("or") + " ";
	similarClubsStr += similarClubs[size - 1]->getName();
	
	description = tr("Is the new club %1 the same as %2?").arg(club->getName()).arg(similarClubsStr);
}
int SimilarClubProblem::getNumSolutions()
{
	return 1 + 2 * similarClubs.size();
}
QString SimilarClubProblem::getSolutionDescription(int i)
{
	if (i == 0) return tr("No");
	
	if (similarClubs.size() == 1)
	{
		if (i == 1) return tr("Yes, change to %1").arg(club->getName());
		else return tr("Yes, keep %1").arg(similarClubs[i - 2]->getName());
	}
	else
	{
		if (i % 2 == 1) return tr("Same as %1, change to %2").arg(similarClubs[i/2]->getName()).arg(club->getName());
		else return tr("Same as %1, keep %2").arg(similarClubs[(i-1)/2]->getName()).arg(similarClubs[(i-1)/2]->getName());
	}
}
void SimilarClubProblem::applySolution(int i)
{
	if (i == 0)
		return;
	
	Club* sameClub = similarClubs[(i-1)/2];
	
	// Change to new name?
	if (i % 2 == 1)
		sameClub->setName(club->getName());
	
	// Change all references to sameClub and delete club from DB
	runnerDB.changeClubForAllRunners(club, sameClub);
	results->changeInColumn(results->getClubColumn(), qVariantFromValue<void*>(club), qVariantFromValue<void*>(sameClub));
	clubDB.deleteClub(club);
}
void SimilarClubProblem::setToSolutionType(Problem::SolutionType type)
{
	if (type == SolutionType_No)
		selection = 0;
	else if (type == SolutionType_Change)
		selection = 1;
	else if (type == SolutionType_Keep)
		selection = 2;
}

// ### SimilarRunnerProblem ###

SimilarRunnerProblem::SimilarRunnerProblem(Runner* runner, std::vector< Runner* >& similarRunners, ResultList* results)
 : runner(runner), similarRunners(similarRunners), results(results)
{
	QString similarRunnersStr = "";
	int size = similarRunners.size();
	for (int i = 0; i < size - 1; ++i)
	{
		similarRunnersStr += similarRunners[i]->getFullDesc();
		if (i < size - 2)
			similarRunnersStr += ", ";
	}
	if (size > 1)
		similarRunnersStr += " " + tr("or") + " ";
	similarRunnersStr += similarRunners[size - 1]->getFullDesc();
	
	description = tr("Is the new runner %1 the same as %2?").arg(runner->getFullDesc()).arg(similarRunnersStr);
}
int SimilarRunnerProblem::getNumSolutions()
{
	return 1 + 2 * similarRunners.size();
}
QString SimilarRunnerProblem::getSolutionDescription(int i)
{
	if (i == 0) return tr("No");
	
	if (similarRunners.size() == 1)
	{
		if (i == 1) return tr("Yes, change to %1").arg(runner->getFullDesc());
		else return tr("Yes, keep %1").arg(similarRunners[i - 2]->getFullDesc());
	}
	else
	{
		if (i % 2 == 1) return tr("Same as %1, change to %2").arg(similarRunners[i/2]->getFullDesc()).arg(runner->getFullDesc());
		else return tr("Same as %1, keep %2").arg(similarRunners[(i-1)/2]->getFullDesc()).arg(similarRunners[(i-1)/2]->getFullDesc());
	}
}
void SimilarRunnerProblem::applySolution(int i)
{
	if (i == 0)
		return;
	
	Runner* sameRunner = similarRunners[(i-1)/2];
	
	// Change to new runner?
	if (i % 2 == 1)
	{
		sameRunner->setFirstName(runner->getFirstName());
		sameRunner->setLastName(runner->getLastName());
		sameRunner->setYear(runner->getYear());
		sameRunner->setIsMale(runner->isMale());
		for (int i = 0; i < runner->getNumClubs(); ++i)
			sameRunner->addClub(runner->getClub(i));
	}
	
	// Change all references to sameRunner and delete runner from DB
	for (int k = 0; k < results->columnCount(); ++k)
	{
		if (results->getColumnType(k) == ResultList::ColumnRunner)
			results->changeInColumn(k, qVariantFromValue<void*>(runner), qVariantFromValue<void*>(sameRunner));
	}
	runnerDB.deleteRunner(runner);
}
void SimilarRunnerProblem::setToSolutionType(Problem::SolutionType type)
{
	if (type == SolutionType_No)
		selection = 0;
	else if (type == SolutionType_Change)
		selection = 1;
	else if (type == SolutionType_Keep)
		selection = 2;
}
