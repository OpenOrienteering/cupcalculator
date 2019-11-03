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


#ifndef CUPCALCULATOR_SCORINGDIALOG_H
#define CUPCALCULATOR_SCORINGDIALOG_H

#include <QtGlobal>
#include <QDialog>
#include <QListWidget>
#include <QObject>
#include <QSize>
#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QButtonGroup;
class QCheckBox;
class QCloseEvent;
class QComboBox;
class QDropEvent;
class QGroupBox;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSortFilterProxyModel;
class QStackedLayout;
class QStackedWidget;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

class AdjustCategoriesPage;
class CustomCategory;
class Scoring;
class ScoringCalculationPage;
struct Ruleset;

class ScoringDialog : public QDialog
{
Q_OBJECT
public:
	
	ScoringDialog(Scoring* _scoring, QWidget* parent = nullptr);
	~ScoringDialog();
	
	void createSteps();
	void updateWindowTitle();
	void setCustomCategories(bool enable);
	void rulesetsChanged();
	
	// Overridden methods
	virtual void closeEvent(QCloseEvent* event);
    virtual QSize sizeHint() const;
	
public slots:
	
	void changePage(QListWidgetItem* current, QListWidgetItem* previous);
		
private:
	
	void clearSteps();
	QListWidgetItem* createStep(const QString& text, QWidget* page);
	
	QListWidget* stepsList;
	QStackedWidget* pagesWidget;
	QWidget* generalPage;
	QWidget* limitRunnersPage;
	AdjustCategoriesPage* customCategoriesPage;
	ScoringCalculationPage* scoringCalculationPage;
	QWidget* teamScoringPage;
	
	int next_step_number;
	QString old_file_name;
	
	Scoring* scoring;
};

class GeneralPage : public QWidget
{
Q_OBJECT
public:
	
	GeneralPage(Scoring* _scoring, ScoringDialog* _dialog, QWidget* parent = nullptr);
	
public slots:
	
	void checkToggled(bool checked);
	void nameChanged();
	void decimalPlacesChanged();
	
private:
	
	QLineEdit* nameEdit;
	QLineEdit* decimalPlacesEdit;
	QCheckBox* limitRunnersCheck;
	QCheckBox* customCategoriesCheck;
	QCheckBox* teamScoringCheck;
	
	Scoring* scoring;
	ScoringDialog* dialog;
};

class PointListWidget : public QListWidget
{
Q_OBJECT
public:
	PointListWidget(ScoringCalculationPage* page, QWidget* parent = nullptr) : QListWidget(parent), page(page) {}
public slots:
	void updateList();
private:
    virtual void dropEvent(QDropEvent* event);
	ScoringCalculationPage* page;
};

class ScoringCalculationPage : public QWidget
{
Q_OBJECT
friend class PointListWidget;
public:
	
	ScoringCalculationPage(Scoring* _scoring, ScoringDialog* _dialog, QWidget* parent = nullptr);
    virtual ~ScoringCalculationPage();
	
	/// Shows or hides the ruleset part of this page
	void setCustomCategories(bool enable);
	
public slots:
	
	void ruleTypeChanged(bool checked);
	void formulaNumberChanged(int i);
	void ratio1Changed();
	void ratio2Changed();
	void ratio3Changed();
    void rulesetChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void intervalChanged();
	void lastRunnerPointsChanged();
	void countingRunnersPerClubChanged();
	void disqualifiedRunnersCountToggled(bool checked);
	void winnerPointCalculationChanged(int id);
	void addRulesetClicked();
	void deleteRulesetClicked();
	void rulesetRenamed(QListWidgetItem* item);
	void customPointAddClicked();
	void customPointRemoveClicked();
    void customPointItemChanged(QListWidgetItem* item);
	void timePointAddClicked();
	void timePointRemoveClicked();
	void timePointCellChanged(int row, int column);
	void timePointCurrentCellChanged(int current_row, int current_column, int previous_row, int previous_column);
    void handicapCheckToggled(bool checked);
    void currentHandicapChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void handicapDataChanged();
	void handicapDataChanged(int index);
	void handicapAddClicked();
	void handicapRemoveClicked();
	
private:
	
	void updateHandicapWidgets();
	void timePointUpdateRow(int row);
	
	Ruleset* getCurrentRuleset();
	
	QGroupBox* rulesetGroup;
	QListWidget* rulesetList;
	QPushButton* deleteRulesetButton;
	
	QButtonGroup* calculationButtonGroup;
	QRadioButton* fixedIntervalRadio;
	QRadioButton* customTableRadio;
	QRadioButton* timeRatioRadio;
	QRadioButton* timePointsRadio;
	
	QStackedLayout* settingsStack;
	
	// time ratio
	QButtonGroup* ratioButtonGroup;
	QRadioButton* ratio_1;
	QLineEdit* ratio_1_factor;
	QRadioButton* ratio_2;
	QLineEdit* ratio_2_factor;
	QRadioButton* ratio_3;
	QLineEdit* ratio_3_bias;
	QLineEdit* ratio_3_factor;
	QLineEdit* ratio_3_average_percentage;
	
	// fixed interval
	QLineEdit* intervalEdit;
	QLineEdit* lastRunnerPointsEdit;
	
	QLineEdit* countingRunnersPerClubEdit;
	QCheckBox* disqualifiedRunnersCountCheck;
	QButtonGroup* winnerPointButtonGroup;
	QRadioButton* winnerPointRadio[2];
	
	QLineEdit* numSpecialRulesEdit;
	
	// custom point table
	PointListWidget* customPointList;
	QPushButton* customPointAdd;
	QPushButton* customPointRemove;
	
	// time point table
	QTableWidget* timePointTable;
	QPushButton* timePointAdd;
	QPushButton* timePointRemove;
	
	// handicap
	QCheckBox* handicapCheck;
	QGroupBox* handicapGroup;
	QListWidget* handicapList;
	QComboBox* handicapGenderCombo;
	QLineEdit* handicapStart;
	QLineEdit* handicapEnd;
	QLineEdit* handicapFactor;
	QPushButton* handicapAdd;
	QPushButton* handicapRemove;
	
	bool react_to_changes;
	
	Scoring* scoring;
	ScoringDialog* dialog;
};

class LimitRunnersPage : public QWidget
{
Q_OBJECT
public:
	
	LimitRunnersPage(Scoring* _scoring, ScoringDialog* _dialog, QWidget* parent = nullptr);
	virtual ~LimitRunnersPage();
	
public slots:
	
	void limitRegionsChanged(bool checked);
	void locationChecked(QTreeWidgetItem* item, int column);
	void limitClubsChanged(bool checked);
	void clubChecked(QListWidgetItem* item);
	
private:
	
	QCheckBox* limitRegionsCheck;
	QTreeWidget* locationWidget;
	
	QCheckBox* limitClubsCheck;
	QSortFilterProxyModel* clubSortedModel;
	QListWidget* clubWidget;
	
	Scoring* scoring;
	ScoringDialog* dialog;
};

class AdjustCategoriesPage : public QWidget
{
Q_OBJECT
public:
	
	AdjustCategoriesPage(Scoring* _scoring, ScoringDialog* _dialog, QWidget* parent = nullptr);
	virtual ~AdjustCategoriesPage();
	
	void updateRulesetCombo(bool keepSelection);
	
public slots:
	
	void addCategoryClicked();
	void deleteCategoryClicked();
	void categoryChanged(QListWidgetItem* current, QListWidgetItem* previous);
	
	void nameChanged(QString text);
    void sourcesChanged();
    void rulesetChanged(int index);
	
private:
	
	CustomCategory* getCurrentCategory();
	
	QListWidget* categoriesList;
	QPushButton* deleteCategoryButton;
	QLineEdit* nameEdit;
	QLineEdit* sourceEdit;
	QComboBox* rulesetCombo;
	
	bool react_to_changes;
	
	Scoring* scoring;
	ScoringDialog* dialog;
};

class TeamScoringPage : public QWidget
{
Q_OBJECT
public:
	
	TeamScoringPage(Scoring* _scoring, ScoringDialog* _dialog, QWidget* parent = nullptr);
	virtual ~TeamScoringPage();
	
public slots:
	
	void excludeChanged();
    void showSingleResultsChecked(bool checked);
	
private:
	
	QLineEdit* excludeEdit;
	QCheckBox* showSingleResultsCheck;
	
	Scoring* scoring;
	ScoringDialog* dialog;
};

/// Empty page which does nothing, can be used as placeholder
class EmptyPage : public QWidget
{
Q_OBJECT
public:
	
	EmptyPage(Scoring* _scoring, ScoringDialog* _dialog, QWidget* parent = nullptr);
	
private:
	
	Scoring* scoring;
	ScoringDialog* dialog;
};

#endif
