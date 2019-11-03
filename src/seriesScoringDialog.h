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


#ifndef CUPCALCULATOR_SERIESSCORINGDIALOG_H
#define CUPCALCULATOR_SERIESSCORINGDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QListWidget;
class QLineEdit;
class QListWidgetItem;
class QStackedWidget;
class QButtonGroup;
class QCheckBox;
class QComboBox;
QT_END_NAMESPACE

class SeriesScoring;
struct SeriesRaceResult;
class ResultList;
class ResultsTable;
class SeriesRacesPage;
class SeriesResultsPage;

class SeriesScoringDialog : public QDialog
{
Q_OBJECT
public:
	
	SeriesScoringDialog(SeriesScoring* _scoring, QWidget* parent = nullptr);
	~SeriesScoringDialog();
	
	void jumpToRaceResult(SeriesRaceResult* result);
	
	void createSteps();
	void updateWindowTitle();
	
	// Overridden methods
	virtual void closeEvent(QCloseEvent* event);
	virtual QSize sizeHint() const;
	
public slots:
	
	void changePage(QListWidgetItem* current, QListWidgetItem* previous);
	
private:
	
	QListWidgetItem* createStep(const QString& text, QWidget* page);
	
	QListWidget* stepsList;
	QStackedWidget* pagesWidget;
	QWidget* settingsPage;
	SeriesRacesPage* racesPage;
	SeriesResultsPage* resultPage;
	
	int next_step_number;
	QString old_file_name;
	
	SeriesScoring* scoring;
};

class SeriesSettingsPage : public QWidget
{
Q_OBJECT
public:
	
	SeriesSettingsPage(SeriesScoring* _scoring, SeriesScoringDialog* _dialog, QWidget* parent = nullptr);
	
public slots:
	
	void nameChanged();
    void organizerBonusTypeChanged(int id);
    void onlyBestRunsCountCheckToggled(bool checked);
    void countingRunsChanged();
	void fixedOrganizerBonusChanged();
	void percentageOrganizerBonusChanged();
	void percentageOrganizerBonusCountingRunsChanged();
	
private:
	
	void updateWidgetActivations();
	
	QLineEdit* nameEdit;
	QCheckBox* only_best_runs_count_check;
	QLineEdit* countingRunsEdit;
	QButtonGroup* organizerButtonGroup;
	QLineEdit* fixedOrganizerBonusAmount;
	QLineEdit* percentageOrganizerBonusAmount;
	QLineEdit* percentageOrganizerBonusCountingRuns;
	
	SeriesScoring* scoring;
	SeriesScoringDialog* dialog;
};

class SeriesRacesPage : public QWidget
{
Q_OBJECT
public:
	
	SeriesRacesPage(SeriesScoring* _scoring, SeriesScoringDialog* _dialog, QWidget* parent = nullptr);
	
	void jumpToRaceResult(SeriesRaceResult* result);
	
public slots:
	
	void updateResults();
    void currentRaceChanged(int index);
    void nameChanged();
    void currentOrganizerChanged(QListWidgetItem* current, QListWidgetItem* previous);
	void changeRaceNumberClicked();
	void deleteRaceClicked();
	void editOrganizersClicked();
    void organizerChecked(QListWidgetItem* item);
	
private:
	
	void showOrganizers();
	void showEditOrganizersList();
	void updateRaceResultDisplay();
	
	QComboBox* racesCombo;
	QPushButton* changeRaceNumberButton;
	QPushButton* deleteRaceButton;
	QLineEdit* nameEdit;
	ResultsTable* resultsTable;
	QListWidget* organizersList;
	QPushButton* editOrganizersButton;
	
	bool editOrganizers;
	
	SeriesScoring* scoring;
	SeriesScoringDialog* dialog;
};

class SeriesResultsPage : public QWidget
{
Q_OBJECT
public:
	
	SeriesResultsPage(SeriesScoring* _scoring, SeriesScoringDialog* _dialog, QWidget* parent = nullptr);
	
	void activated();
	
public slots:
	
	void recalculateClicked();
	void exportClicked();
	
private:
	
	ResultsTable* results;
	QPushButton* recalculateButton;
	QLabel* notUpToDateLabel;
	QPushButton* exportButton;
	
	SeriesScoring* scoring;
	SeriesScoringDialog* dialog;
};

class AddToSeriesScoringDialog : public QDialog
{
Q_OBJECT
public:
	
	AddToSeriesScoringDialog(ResultList* _result_list, QWidget* parent = nullptr);
	inline SeriesRaceResult* getNewSeriesRaceResult() {return new_result;}
	inline SeriesScoring* getAddedToScoring() {return added_to_scoring;}
	
public slots:
	
	void currentSeriesChanged(int index);
	void addClicked();
	
private:
	
	void suggestRaceNumber();
    void updateRacesList();
	
	QComboBox* seriesCombo;
	QListWidget* racesList;
	QLineEdit* raceNumberEdit;
	QLineEdit* nameEdit;
	QLabel* statusLabel;
	QPushButton* addButton;
	
	ResultList* result_list;
	
	SeriesRaceResult* new_result;
	SeriesScoring* added_to_scoring;
};

class ChangeRaceNumberDialog : public QDialog
{
Q_OBJECT
public:
	
	ChangeRaceNumberDialog(SeriesScoring* scoring, const QString& race_name, QWidget* parent = nullptr);
	
	inline int getNumber() const {return number;}
	
public slots:
	
    void numberChanged(QString new_text);
	
private:
	
	QPushButton* changeButton;
	QLineEdit* edit;
	int number;
	SeriesScoring* scoring;
};

#endif
