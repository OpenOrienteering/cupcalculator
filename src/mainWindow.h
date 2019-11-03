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


#ifndef CUPCALCULATOR_MAINWINDOW_H
#define CUPCALCULATOR_MAINWINDOW_H

#include <QDialog>
#include "problemWidget.h"

QT_BEGIN_NAMESPACE
class QTabWidget;
class QListWidget;
class QComboBox;
class QTableWidget;
class QTableView;
class QListView;
class QSortFilterProxyModel;
class QListWidgetItem;
class QLineEdit;
QT_END_NAMESPACE

class Event;
class ClubTable;
class RunnerTable;
class ResultsTable;
class ResultList;
class Club;
class Runner;
class CalculateScoringTab;
class CSVFile;

class MainWindow : public QDialog
{
Q_OBJECT
public:
	
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();
	
	bool importResultsFromCSV(CSVFile* file, bool show_import_dialog = true);
	
    virtual QSize sizeHint() const;
	
	inline Event* getEvent() {return event;}
	
private:
	
	Event* event;
	
	QTabWidget* tabWidget;
	CalculateScoringTab* calculate_scoring_tab;
};

class CalculateScoringTab : public QWidget
{
Q_OBJECT
public:
	
	CalculateScoringTab(Event* _event, QWidget* parent = nullptr);
	
	bool importResultsFromCSV(CSVFile* file, bool show_import_dialog = true);
	
public slots:
	
	void newScoringClicked();
	void editScoringClicked();
	void removeScoringClicked();
	void currentScoringChanged(QListWidgetItem* current, QListWidgetItem* previous);
	void scoringChecked(QListWidgetItem* item);
	void scoringDoubleClicked(QListWidgetItem* item);
	
	void importResultsClicked();
	void closeEventClicked();
	void calculateScoringsClicked();
	void presentScoringClicked();
	
	void addToSeriesScoringClicked();
	void exportScoringsClicked();
	void viewScoringListChanged(int index);
	
private:
	
	void updateCalculateScoringButton();
	int getCorrectedTime(const QString& time_text);
	
	Event* event;
	
	QListWidget* scoringsList;
	QPushButton* newScoringButton;
	QPushButton* editScoringButton;
	QPushButton* deleteScoringButton;
	
	QPushButton* importResultsButton;
	QPushButton* closeEventButton;
	ResultsTable* resultsTable;
	QLineEdit* eventYearEdit;
	QPushButton* calculateScoringsButton;
	QPushButton* presentScoringButton;
	
	QComboBox* viewScoringCombo;
	ResultsTable* scoringTable;
	QPushButton* addToSeriesScoringButton;
	QPushButton* exportScoringButton;
};

class SeriesScoringTab : public QWidget
{
Q_OBJECT
public:
	
	SeriesScoringTab(QWidget* parent = nullptr);
	
public slots:
	
	void addClicked();
	void deleteClicked();
	void editClicked();
	
	void currentSeriesChanged(QListWidgetItem* current, QListWidgetItem* previous);
	void seriesDoubleClicked(QListWidgetItem* item);
	
private:
	
	QListWidget* seriesList;
	QPushButton* editButton;
	QPushButton* deleteButton;
};

class ClubDatabaseTab : public QWidget
{
Q_OBJECT
public:
	
	ClubDatabaseTab(MainWindow* mainWindow, QWidget* parent = nullptr);
	
public slots:
	
	void editClicked();
	void addClicked();
	void removeClicked();
	void mergeClicked();
	void updateActions();
	
private:
	
	ClubTable* clubList;
	
	QPushButton* editButton;
	QPushButton* addButton;
	QPushButton* removeButton;
	QPushButton* mergeButton;
	
	MainWindow* mainWindow;
};

class RunnerDatabaseTab : public QWidget
{
Q_OBJECT
public:
	
	RunnerDatabaseTab(MainWindow* mainWindow, QWidget* parent = nullptr);
    virtual ~RunnerDatabaseTab();
	
public slots:
	
	void addClicked();
	void removeClicked();
	void mergeClicked();
	void updateActions();
	void clubSelectionChanged();
	
private:
	
	RunnerTable* runnerList;
	QPushButton* addButton;
	QPushButton* removeButton;
	QPushButton* mergeButton;
	
	QListView* clubView;
	QSortFilterProxyModel* sortedClubModel;
	
	MainWindow* mainWindow;
};

class AboutTab : public QWidget
{
Q_OBJECT
public:
	AboutTab(MainWindow* mainWindow, QWidget* parent = nullptr);
	virtual ~AboutTab();
public slots:
	void linkClicked(QString link);
private:
	MainWindow* mainWindow;
};

class SimilarClubProblem : public Problem
{
Q_OBJECT
public:
	SimilarClubProblem(Club* club, std::vector<Club*>& similarClubs, ResultList* results);
	virtual int getNumSolutions();
	virtual QString getSolutionDescription(int i);
	virtual void applySolution(int i);
    virtual void setToSolutionType(SolutionType type);
private:
	Club* club;
	std::vector<Club*> similarClubs;
	ResultList* results;
};

class SimilarRunnerProblem : public Problem
{
Q_OBJECT
public:
	SimilarRunnerProblem(Runner* runner, std::vector<Runner*>& similarRunners, ResultList* results);
	virtual int getNumSolutions();
	virtual QString getSolutionDescription(int i);
	virtual void applySolution(int i);
    virtual void setToSolutionType(SolutionType type);
private:
	Runner* runner;
	std::vector<Runner*> similarRunners;
	ResultList* results;
};

#endif
