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


#include "scoringDialog.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>
#include <vector>

#include <Qt>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QFlags>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListView>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QRadioButton>
#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <QStackedWidget>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QVariant>

#include "club.h"
#include "config.h"
#include "location.h"
#include "scoring.h"
#include "util.h"

QT_BEGIN_NAMESPACE
class QHBoxLayout;
class QVBoxLayout;
QT_END_NAMESPACE

const QString SCORING_FILE_SUFFIX = "xml";

// ### ScoringDialog ###

ScoringDialog::ScoringDialog(Scoring* _scoring, QWidget* parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint), scoring(_scoring)
{
	old_file_name = scoring->getFileName();
	
	pagesWidget = new QStackedWidget();
	generalPage = new GeneralPage(scoring, this);
	pagesWidget->addWidget(generalPage);
	limitRunnersPage = new LimitRunnersPage(scoring, this);
	pagesWidget->addWidget(limitRunnersPage);
	customCategoriesPage = new AdjustCategoriesPage(scoring, this);
	pagesWidget->addWidget(customCategoriesPage);
	scoringCalculationPage = new ScoringCalculationPage(scoring, this);
	pagesWidget->addWidget(scoringCalculationPage);
	teamScoringPage = new TeamScoringPage(scoring, this);
	pagesWidget->addWidget(teamScoringPage);
	
	stepsList = new QListWidget();
	//stepsList->setViewMode(QListView::IconMode);
	//stepsList->setIconSize(QSize(96, 84));
	//stepsList->setMovement(QListView::Static);
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
	
	connect(stepsList, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
			this, SLOT(changePage(QListWidgetItem*, QListWidgetItem*)));
	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
}
ScoringDialog::~ScoringDialog()
{
}
void ScoringDialog::closeEvent(QCloseEvent* event)
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
		if (scoringDB.contains(scoring->getFileName()))
		{
			QMessageBox::warning(this, APP_NAME, tr("The file name is already in use, please choose a unique name!"));
			event->ignore();
			return;
		}
		else
			scoringDB.changeScoringName(old_file_name, scoring->getFileName());
	}
	
	//scoring->saveToFile();
	event->accept();
}
QSize ScoringDialog::sizeHint() const
{
    return QSize(900, 500);
}
void ScoringDialog::updateWindowTitle()
{
	setWindowTitle(tr("Edit scoring - ") + scoring->getFileName() + "." + SCORING_FILE_SUFFIX);
}
void ScoringDialog::setCustomCategories(bool enable)
{
	scoringCalculationPage->setCustomCategories(enable);
}
void ScoringDialog::rulesetsChanged()
{
	customCategoriesPage->updateRulesetCombo(true);
}
void ScoringDialog::changePage(QListWidgetItem* current, QListWidgetItem* previous)
{
	if (!current)
		current = previous;
	
	pagesWidget->setCurrentWidget(current->data(Qt::UserRole).value<QWidget*>());
}
void ScoringDialog::createSteps()
{
	QWidget* selected_page = nullptr;
	QListWidgetItem* cur_item = stepsList->currentItem();
	if (cur_item)
		selected_page = cur_item->data(Qt::UserRole).value<QWidget*>();
	
	clearSteps();
	createStep(tr("General"), generalPage);
	if (scoring->getLimitRunners())
		createStep(tr("Limit runners"), limitRunnersPage);
	createStep(tr("Scoring calculation"), scoringCalculationPage);
	if (scoring->getCustomCategories())
		createStep(tr("Customize categories"), customCategoriesPage);
	if (scoring->getTeamScoring())
		createStep(tr("Team scoring"), teamScoringPage);
	
	if (selected_page)
	{
		int count = stepsList->count();
		for (int i = 0; i < count; ++i)
		{
			if (stepsList->item(i)->data(Qt::UserRole).value<QWidget*>() == selected_page)
			{
				stepsList->setCurrentRow(i);
				break;
			}
		}
	}
}
void ScoringDialog::clearSteps()
{
	stepsList->clear();
	next_step_number = 1;
}
QListWidgetItem* ScoringDialog::createStep(const QString& text, QWidget* page)
{
	QListWidgetItem* step = new QListWidgetItem(stepsList);
	//step->setIcon(QIcon(":/images/config.png"));
	step->setText(QString::number(next_step_number) + ". " + text);
	step->setTextAlignment(Qt::AlignHCenter);
	step->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	step->setData(Qt::UserRole, qVariantFromValue(page));
	
	++next_step_number;
	
	return step;
}

// ### GeneralPage ###

GeneralPage::GeneralPage(Scoring* _scoring, ScoringDialog* _dialog, QWidget* parent) : QWidget(parent), scoring(_scoring), dialog(_dialog)
{
	QLabel* nameLabel = new QLabel(tr("File name:"));
	nameEdit = new QLineEdit();
	QLabel* nameSuffixLabel = new QLabel("." + SCORING_FILE_SUFFIX);
	
	QHBoxLayout* nameLayout = new QHBoxLayout();
	nameLayout->addWidget(nameLabel);
	nameLayout->addWidget(nameEdit);
	nameLayout->addWidget(nameSuffixLabel);
	
	
	QLabel* decimalPlacesLabel = new QLabel(tr("Decimal places:"));
	decimalPlacesEdit = new QLineEdit();
	decimalPlacesEdit->setValidator(new QIntValidator(0, 10, decimalPlacesEdit));
	
	QHBoxLayout* decimalPlacesLayout = new QHBoxLayout();
	decimalPlacesLayout->addWidget(decimalPlacesLabel);
	decimalPlacesLayout->addWidget(decimalPlacesEdit);
	
	
	limitRunnersCheck = new QCheckBox(tr("Limit runners"));
	limitRunnersCheck->setToolTip(tr("Limit the runners for this scoring - for example, allow only runners from a certain state/province"));
	customCategoriesCheck = new QCheckBox(tr("Customize categories"));
	customCategoriesCheck->setToolTip(tr("Use categories for the scoring which differ from those used in the race, or assign different rulesets to the categories"));
	teamScoringCheck = new QCheckBox(tr("Calculate team scoring"));
	teamScoringCheck->setToolTip(tr("Calculate scorings for teams"));
	
	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addLayout(nameLayout);
	mainLayout->addSpacing(8);
	mainLayout->addLayout(decimalPlacesLayout);
	mainLayout->addSpacing(32);
	mainLayout->addWidget(limitRunnersCheck);
	mainLayout->addWidget(customCategoriesCheck);
	mainLayout->addWidget(teamScoringCheck);
	mainLayout->addStretch(1);
	setLayout(mainLayout);
	
	// Set values
	nameEdit->setText(scoring->getFileName());
	decimalPlacesEdit->setText(QString::number(scoring->getDecimalPlaces()));
	limitRunnersCheck->setChecked(scoring->getLimitRunners());
	customCategoriesCheck->setChecked(scoring->getCustomCategories());
	teamScoringCheck->setChecked(scoring->getTeamScoring());
	
	// Connect signals
	connect(limitRunnersCheck, SIGNAL(toggled(bool)), this, SLOT(checkToggled(bool)));
	connect(customCategoriesCheck, SIGNAL(toggled(bool)), this, SLOT(checkToggled(bool)));
	connect(teamScoringCheck, SIGNAL(toggled(bool)), this, SLOT(checkToggled(bool)));
	connect(nameEdit, SIGNAL(editingFinished()), this, SLOT(nameChanged()));
	connect(decimalPlacesEdit, SIGNAL(editingFinished()), this, SLOT(decimalPlacesChanged()));
}
void GeneralPage::checkToggled(bool /*checked*/)
{
	if (scoring->getCustomCategories() != customCategoriesCheck->isChecked())
		dialog->setCustomCategories(customCategoriesCheck->isChecked());
	
	scoring->setLimitRunners(limitRunnersCheck->isChecked());
	scoring->setCustomCategories(customCategoriesCheck->isChecked());
	scoring->setTeamScoring(teamScoringCheck->isChecked());
	dialog->createSteps();
}
void GeneralPage::nameChanged()
{
	scoring->setFileName(nameEdit->text());
	dialog->updateWindowTitle();
}
void GeneralPage::decimalPlacesChanged()
{
	scoring->setDecimalPlaces(decimalPlacesEdit->text().toInt());
}

// ### ScoringCalculationPage ###

ScoringCalculationPage::ScoringCalculationPage(Scoring* _scoring, ScoringDialog* _dialog, QWidget* parent) : QWidget(parent), scoring(_scoring), dialog(_dialog)
{
	react_to_changes = true;
	
	// Ruleset group
	rulesetGroup = new QGroupBox(tr("Rulesets"));
	rulesetList = new QListWidget();
	QPushButton* addRulesetButton = new QPushButton(QIcon("images/plus.png"), "");
	deleteRulesetButton = new QPushButton(QIcon("images/minus.png"), "");
	
	QGridLayout* rulesetLayout = new QGridLayout();
	rulesetLayout->addWidget(rulesetList, 0, 0, 3, 1);
	rulesetLayout->setRowStretch(2, 1);
	rulesetLayout->addWidget(addRulesetButton, 0, 1);
	rulesetLayout->addWidget(deleteRulesetButton, 1, 1);
	rulesetGroup->setLayout(rulesetLayout);
	
	if (!scoring->getCustomCategories())
		rulesetGroup->hide();
	
	// Rule type group
	QGroupBox* rulesGroup = new QGroupBox(tr("Rule type"));
	timeRatioRadio = new QRadioButton(tr("time ratio"));
	fixedIntervalRadio = new QRadioButton(tr("fixed interval"));
	customTableRadio = new QRadioButton(tr("point table"));
	timePointsRadio = new QRadioButton(tr("time points"));
	
	QVBoxLayout* rulesLayout = new QVBoxLayout();
	rulesLayout->addWidget(timeRatioRadio);
	rulesLayout->addWidget(fixedIntervalRadio);
	rulesLayout->addWidget(customTableRadio);
	rulesLayout->addWidget(timePointsRadio);
	rulesGroup->setLayout(rulesLayout);
	
	calculationButtonGroup = new QButtonGroup();
	calculationButtonGroup->addButton(timeRatioRadio, (int)TimeRatio);
	calculationButtonGroup->addButton(fixedIntervalRadio, (int)FixedInterval);
	calculationButtonGroup->addButton(customTableRadio, (int)PointTable);
	calculationButtonGroup->addButton(timePointsRadio, (int)TimePoints);
	
	// Settings group
	QGroupBox* settingsGroup = new QGroupBox(tr("Settings"));
	
	// Time ratio settings
	QWidget* timeRatioWidget = new QWidget();
	
	ratio_1 = new QRadioButton(tr("(winner_time / time) *"));
	ratio_1_factor = new QLineEdit();
	ratio_1_factor->setValidator(new DoubleValidator(0));
	
	ratio_2 = new QRadioButton(tr("max{0, (2 - time / winner_time)} *"));
	ratio_2_factor = new QLineEdit();
	ratio_2_factor->setValidator(new DoubleValidator(0));
	
	ratio_3 = new QRadioButton(tr("max{0,"));
	ratio_3_bias = new QLineEdit();
	ratio_3_bias->setValidator(new DoubleValidator(-10e10));
	QLabel* ratio_3_label_1 = new QLabel(tr("+"));
	ratio_3_factor = new QLineEdit();
	ratio_3_factor->setValidator(new DoubleValidator(0));
	QLabel* ratio_3_label_2 = new QLabel(tr("* (averaged_time - time) / averaged_time},"));
	QLabel* ratio_3_label_3 = new QLabel(tr("         use averaged time of first"));
	ratio_3_average_percentage = new QLineEdit();
	ratio_3_average_percentage->setValidator(new DoubleValidator(1, 100));
	QLabel* ratio_3_label_4 = new QLabel(tr("% of started competitors (number rounded up)"));
	
	ratioButtonGroup = new QButtonGroup();
	ratioButtonGroup->addButton(ratio_1, 0); 
	ratioButtonGroup->addButton(ratio_2, 1);
	ratioButtonGroup->addButton(ratio_3, 2);
	
	QHBoxLayout* ratio_1_Layout = new QHBoxLayout();
	ratio_1_Layout->addWidget(ratio_1);
	ratio_1_Layout->addWidget(ratio_1_factor);
	QHBoxLayout* ratio_2_Layout = new QHBoxLayout();
	ratio_2_Layout->addWidget(ratio_2);
	ratio_2_Layout->addWidget(ratio_2_factor);
	QHBoxLayout* ratio_3_Layout = new QHBoxLayout();
	ratio_3_Layout->addWidget(ratio_3);
	ratio_3_Layout->addWidget(ratio_3_bias);
	ratio_3_Layout->addWidget(ratio_3_label_1);
	ratio_3_Layout->addWidget(ratio_3_factor);
	ratio_3_Layout->addWidget(ratio_3_label_2);
	QHBoxLayout* ratio_3_Layout_2 = new QHBoxLayout();
	ratio_3_Layout_2->addWidget(ratio_3_label_3);
	ratio_3_Layout_2->addWidget(ratio_3_average_percentage);
	ratio_3_Layout_2->addWidget(ratio_3_label_4);
	
	QVBoxLayout* timeRatioLayout = new QVBoxLayout();
	timeRatioLayout->addLayout(ratio_1_Layout);
	timeRatioLayout->addLayout(ratio_2_Layout);
	timeRatioLayout->addLayout(ratio_3_Layout);
	timeRatioLayout->addLayout(ratio_3_Layout_2);
	timeRatioLayout->addStretch(1);
	timeRatioWidget->setLayout(timeRatioLayout);
	
	// Fixed interval settings
	QWidget* fixedIntervalWidget = new QWidget();
	
	QLabel* intervalLabel = new QLabel(tr("Point interval:"));
	intervalEdit = new QLineEdit();
	intervalEdit->setValidator(new DoubleValidator(0));
	QHBoxLayout* intervalLayout = new QHBoxLayout();
	intervalLayout->addWidget(intervalLabel);
	intervalLayout->addWidget(intervalEdit);
	
	QLabel* lastRunnerPointsLabel = new QLabel(tr("Points given to last runner:"));
	lastRunnerPointsEdit = new QLineEdit();
	lastRunnerPointsEdit->setValidator(new DoubleValidator(0));
	QHBoxLayout* lastRunnerPointsLayout = new QHBoxLayout();
	lastRunnerPointsLayout->addWidget(lastRunnerPointsLabel);
	lastRunnerPointsLayout->addWidget(lastRunnerPointsEdit);
	
	QLabel* countingRunnersPerClubLabel = new QLabel(tr("Counting runners per club:"));
	countingRunnersPerClubEdit = new QLineEdit();
	countingRunnersPerClubEdit->setValidator(new QIntValidator(1, 999999, countingRunnersPerClubEdit));
	QHBoxLayout* countingRunnersPerClubLayout = new QHBoxLayout();
	countingRunnersPerClubLayout->addWidget(countingRunnersPerClubLabel);
	countingRunnersPerClubLayout->addWidget(countingRunnersPerClubEdit);
	
	disqualifiedRunnersCountCheck = new QCheckBox(tr("Disqualified runners count for other runners' points"));
	winnerPointRadio[0] = new QRadioButton(tr("Determine points of winner by number of participating counting runners"));
	winnerPointRadio[1] = new QRadioButton(tr("Determine points of winner by number of participating teams * maximum counting runners per team"));
	
	winnerPointButtonGroup = new QButtonGroup();
	winnerPointButtonGroup->addButton(winnerPointRadio[0], 0);
	winnerPointButtonGroup->addButton(winnerPointRadio[1], 1);
	
	QVBoxLayout* fixedIntervalLayout = new QVBoxLayout();
	fixedIntervalLayout->addLayout(intervalLayout);
	fixedIntervalLayout->addLayout(lastRunnerPointsLayout);
	fixedIntervalLayout->addLayout(countingRunnersPerClubLayout);
	fixedIntervalLayout->addWidget(winnerPointRadio[0]);
	fixedIntervalLayout->addWidget(winnerPointRadio[1]);
	fixedIntervalLayout->addWidget(disqualifiedRunnersCountCheck);
	fixedIntervalWidget->setLayout(fixedIntervalLayout);
	
	// Custom table settings
	QWidget* customTableWidget = new QWidget();
	
	customPointList = new PointListWidget(this);
	customPointList->setDragEnabled(true);
	customPointList->setDefaultDropAction(Qt::MoveAction);
	customPointList->setDropIndicatorShown(true);
	customPointList->setDragDropMode(QAbstractItemView::DragDrop);
	customPointList->setDragDropOverwriteMode(false);
	customPointAdd = new QPushButton(QIcon("images/plus.png"), "");
	customPointRemove = new QPushButton(QIcon("images/minus.png"), "");
	QLabel* customPointLabel = new QLabel(tr("The first runner will get the number of points in the topmost row,\nthe second one the number of points in the next row, and so on.\nIf there are more runners than rows, the points from the last row are given to those runners.\nDrag the items with the mouse to reorder them, double click to edit."));

	QGridLayout* customTableLayout = new QGridLayout();
	customTableLayout->addWidget(customPointList, 0, 0, 3, 1);
	customTableLayout->setRowStretch(2, 1);
	customTableLayout->addWidget(customPointAdd, 0, 1);
	customTableLayout->addWidget(customPointRemove, 1, 1);
	customTableLayout->addWidget(customPointLabel, 3, 0, 1, 2);
	customTableWidget->setLayout(customTableLayout);
	
	// Time point settings
	QWidget* timePointWidget = new QWidget();
	
	timePointTable = new QTableWidget();
	timePointTable->setColumnCount(2);
	
	timePointTable->setEditTriggers(QAbstractItemView::AllEditTriggers);
	timePointTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	timePointTable->setHorizontalHeaderLabels(QStringList() << tr("Percentage of winner time") << tr("Points"));
	timePointTable->verticalHeader()->setVisible(false);
	QHeaderView* header_view = timePointTable->horizontalHeader();
	for (int i = 0; i < 2; ++i)
		header_view->setSectionResizeMode(i, QHeaderView::ResizeToContents);
	header_view->setSectionsClickable(false);
	
	timePointAdd = new QPushButton(QIcon("images/plus.png"), "");
	timePointRemove = new QPushButton(QIcon("images/minus.png"), "");
	QLabel* timePointLabel = new QLabel(tr("The runners get points according to the percentage their time is behind the winner time.\nIf a runner is further behind than the percentage in the last row,\nhe or she still gets the points from this row."));
	
	QGridLayout* timePointLayout = new QGridLayout();
	timePointLayout->addWidget(timePointTable, 0, 0, 3, 1);
	timePointLayout->setRowStretch(2, 1);
	timePointLayout->addWidget(timePointAdd, 0, 1);
	timePointLayout->addWidget(timePointRemove, 1, 1);
	timePointLayout->addWidget(timePointLabel, 3, 0, 1, 2);
	timePointWidget->setLayout(timePointLayout);
	
	// Settings stack
	settingsStack = new QStackedLayout(this);
	settingsStack->addWidget(timeRatioWidget);
	settingsStack->addWidget(fixedIntervalWidget);
	settingsStack->addWidget(customTableWidget);
	settingsStack->addWidget(timePointWidget);
	settingsGroup->setLayout(settingsStack);
	
	// Handicap
	handicapCheck = new QCheckBox(tr("Handicapping"));
	handicapCheck->setToolTip(tr("Compensate times of runners based on their age and sex"));
	
	handicapGroup = new QGroupBox(tr("Handicap groups"));
	handicapList = new QListWidget();
	handicapAdd = new QPushButton(QIcon("images/plus.png"), "");
	handicapRemove = new QPushButton(QIcon("images/minus.png"), "");
	
	QLabel* handicapGenderLabel = new QLabel(tr("Apply to:"));
	handicapGenderCombo = new QComboBox();
	handicapGenderCombo->addItem(tr("Women"), false);
	handicapGenderCombo->addItem(tr("Men"), true);
	QLabel* handicapStartLabel = new QLabel(tr("Apply from age"));
	handicapStart = new QLineEdit();
	handicapStart->setValidator(new QIntValidator(0, 999999, handicapStart));
	QLabel* handicapEndLabel = new QLabel(tr("to"));
	handicapEnd = new QLineEdit();
	handicapEnd->setValidator(new QIntValidator(0, 999999, handicapStart));
	QLabel* handicapFactorLabel = new QLabel(tr("Factor:"));
	handicapFactor = new QLineEdit();
	handicapFactor->setValidator(new DoubleValidator(0));
	
	QGridLayout* handicapListLayout = new QGridLayout();
	handicapListLayout->addWidget(handicapList, 0, 0, 3, 1);
	handicapListLayout->addWidget(handicapAdd, 0, 1);
	handicapListLayout->addWidget(handicapRemove, 1, 1);
	handicapListLayout->setRowStretch(2, 1);
	QHBoxLayout* handicapGenderLayout = new QHBoxLayout();
	handicapGenderLayout->addWidget(handicapGenderLabel);
	handicapGenderLayout->addWidget(handicapGenderCombo);
	QHBoxLayout* handicapStartEndLayout = new QHBoxLayout();
	handicapStartEndLayout->addWidget(handicapStartLabel);
	handicapStartEndLayout->addWidget(handicapStart);
	handicapStartEndLayout->addWidget(handicapEndLabel);
	handicapStartEndLayout->addWidget(handicapEnd);
	QHBoxLayout* handicapFactorLayout = new QHBoxLayout();
	handicapFactorLayout->addWidget(handicapFactorLabel);
	handicapFactorLayout->addWidget(handicapFactor);
	
	QVBoxLayout* handicapLayout = new QVBoxLayout();
	handicapLayout->addLayout(handicapListLayout);
	handicapLayout->addLayout(handicapGenderLayout);
	handicapLayout->addLayout(handicapStartEndLayout);
	handicapLayout->addLayout(handicapFactorLayout);
	handicapGroup->setLayout(handicapLayout);
	
	// Layout
	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addWidget(rulesetGroup);
	mainLayout->addWidget(handicapCheck);
	mainLayout->addWidget(handicapGroup);
	mainLayout->addWidget(rulesGroup);
	mainLayout->addWidget(settingsGroup);
	mainLayout->addStretch(1);
	setLayout(mainLayout);
	
	// Set values
	int size = scoring->getNumRulesets();
	for (int i = 0; i < size; ++i)
	{
		Ruleset* ruleset = scoring->getRuleset(i);
		
		QListWidgetItem* item = new QListWidgetItem(rulesetList);
		item->setText(ruleset->name);
		item->setFlags(Qt::ItemIsSelectable | ((i == 0) ? Qt::NoItemFlags : Qt::ItemIsEditable) | Qt::ItemIsEnabled);
		item->setData(Qt::UserRole, qVariantFromValue<void*>(ruleset));
		
		rulesetList->addItem(item);
	}
	
	// Connections
	connect(addRulesetButton, SIGNAL(clicked()), this, SLOT(addRulesetClicked()));
	connect(deleteRulesetButton, SIGNAL(clicked()), this, SLOT(deleteRulesetClicked()));
	connect(rulesetList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(rulesetChanged(QListWidgetItem*,QListWidgetItem*)));
	connect(rulesetList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(rulesetRenamed(QListWidgetItem*)));
	
	connect(timeRatioRadio, SIGNAL(toggled(bool)), this, SLOT(ruleTypeChanged(bool)));
	connect(fixedIntervalRadio, SIGNAL(toggled(bool)), this, SLOT(ruleTypeChanged(bool)));
	connect(customTableRadio, SIGNAL(toggled(bool)), this, SLOT(ruleTypeChanged(bool)));
	
	connect(ratioButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(formulaNumberChanged(int)));
	connect(ratio_1_factor, SIGNAL(editingFinished()), this, SLOT(ratio1Changed()));
	connect(ratio_2_factor, SIGNAL(editingFinished()), this, SLOT(ratio2Changed()));
	connect(ratio_3_factor, SIGNAL(editingFinished()), this, SLOT(ratio3Changed()));
	connect(ratio_3_bias, SIGNAL(editingFinished()), this, SLOT(ratio3Changed()));
	connect(ratio_3_average_percentage, SIGNAL(editingFinished()), this, SLOT(ratio3Changed()));
	
	connect(intervalEdit, SIGNAL(editingFinished()), this, SLOT(intervalChanged()));
	connect(lastRunnerPointsEdit, SIGNAL(editingFinished()), this, SLOT(lastRunnerPointsChanged()));
	connect(countingRunnersPerClubEdit, SIGNAL(editingFinished()), this, SLOT(countingRunnersPerClubChanged()));
	connect(disqualifiedRunnersCountCheck, SIGNAL(toggled(bool)), this, SLOT(disqualifiedRunnersCountToggled(bool)));
	connect(winnerPointButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(winnerPointCalculationChanged(int)));
	
	connect(customPointAdd, SIGNAL(clicked()), this, SLOT(customPointAddClicked()));
	connect(customPointRemove, SIGNAL(clicked()), this, SLOT(customPointRemoveClicked()));
	connect(customPointList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(customPointItemChanged(QListWidgetItem*)));
	
	connect(timePointAdd, SIGNAL(clicked()), this, SLOT(timePointAddClicked()));
	connect(timePointRemove, SIGNAL(clicked()), this, SLOT(timePointRemoveClicked()));
	connect(timePointTable, SIGNAL(cellChanged(int,int)), this, SLOT(timePointCellChanged(int,int)));
	connect(timePointTable, SIGNAL(currentCellChanged(int,int,int,int)), this, SLOT(timePointCurrentCellChanged(int,int,int,int)));
	
	connect(handicapCheck, SIGNAL(toggled(bool)), this, SLOT(handicapCheckToggled(bool)));
	connect(handicapList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(currentHandicapChanged(QListWidgetItem*,QListWidgetItem*)));
	connect(handicapAdd, SIGNAL(clicked()), this, SLOT(handicapAddClicked()));
	connect(handicapRemove, SIGNAL(clicked()), this, SLOT(handicapRemoveClicked()));
	connect(handicapStart, SIGNAL(editingFinished()), this, SLOT(handicapDataChanged()));
	connect(handicapEnd, SIGNAL(editingFinished()), this, SLOT(handicapDataChanged()));
	connect(handicapGenderCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(handicapDataChanged(int)));
	connect(handicapFactor, SIGNAL(editingFinished()), this, SLOT(handicapDataChanged()));
	
	rulesetList->setCurrentRow(0);
}
ScoringCalculationPage::~ScoringCalculationPage()
{
	delete calculationButtonGroup;
	delete ratioButtonGroup;
}

void ScoringCalculationPage::handicapCheckToggled(bool checked)
{
	if (!react_to_changes) return;
	Ruleset* ruleset = getCurrentRuleset();
	
	ruleset->handicapping = checked;
	handicapGroup->setVisible(checked);
}
void ScoringCalculationPage::handicapAddClicked()
{
	Ruleset* ruleset = getCurrentRuleset();
	int cur_index = handicapList->currentRow();
	
	Ruleset::HandicapSetting* new_handicap = new Ruleset::HandicapSetting();
	new_handicap->ageStart = 0;
	new_handicap->ageEnd = 99;
	new_handicap->forMale = false;
	new_handicap->factor = 1;
	ruleset->handicaps.insert(ruleset->handicaps.begin() + (cur_index + 1), new_handicap);
	
	QListWidgetItem* new_item = new QListWidgetItem(new_handicap->getDesc());
	new_item->setData(Qt::UserRole, qVariantFromValue<void*>(new_handicap));
	handicapList->insertItem(cur_index + 1, new_item);
	handicapList->setCurrentRow(cur_index + 1);
	handicapList->sortItems();
}
void ScoringCalculationPage::handicapRemoveClicked()
{
	Ruleset* ruleset = getCurrentRuleset();
	int cur_index = handicapList->currentRow();
	
	delete handicapList->item(cur_index);
	ruleset->handicaps.erase(ruleset->handicaps.begin() + cur_index);
	
	updateHandicapWidgets();
}
void ScoringCalculationPage::currentHandicapChanged(QListWidgetItem* current, QListWidgetItem* /*previous*/)
{
	updateHandicapWidgets();
	
	if (!current)
		return;
	
	react_to_changes = false;
	Ruleset::HandicapSetting* handicap = reinterpret_cast<Ruleset::HandicapSetting*>(current->data(Qt::UserRole).value<void*>());
	handicapStart->setText(QString::number(handicap->ageStart));
	handicapEnd->setText(QString::number(handicap->ageEnd));
	handicapFactor->setText(QString::number(handicap->factor));
	handicapGenderCombo->setCurrentIndex(handicapGenderCombo->findData(handicap->forMale, Qt::UserRole));
	react_to_changes = true;
}
void ScoringCalculationPage::handicapDataChanged()
{
	if (!react_to_changes) return;
	
	QListWidgetItem* currentItem = handicapList->currentItem();
	Ruleset::HandicapSetting* handicap = reinterpret_cast<Ruleset::HandicapSetting*>(currentItem->data(Qt::UserRole).value<void*>());
	int age1 = handicapStart->text().toInt();
	int age2 = handicapEnd->text().toInt();
	if (age1 < age2)
	{
		handicap->ageStart = age1;
		handicap->ageEnd = age2;
	}
	else
	{
		handicap->ageStart = age2;
		handicap->ageEnd = age1;
	}
	handicap->factor = handicapFactor->text().toDouble();
	handicap->forMale = handicapGenderCombo->itemData(handicapGenderCombo->currentIndex(), Qt::UserRole).toBool();
	
	currentItem->setText(handicap->getDesc());
	handicapList->sortItems();
}

void ScoringCalculationPage::customPointAddClicked()
{
	Ruleset* ruleset = getCurrentRuleset();
	int cur_index = customPointList->currentRow();
	
	QListWidgetItem* new_item = new QListWidgetItem("1");
	new_item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
	customPointList->insertItem(cur_index + 1, new_item);
	customPointList->setCurrentRow(cur_index + 1);
	customPointList->editItem(customPointList->item(cur_index + 1));
	
	ruleset->pointTableSettings.table.insert(ruleset->pointTableSettings.table.begin() + (cur_index + 1), FPNumber(1.0));
	
	customPointRemove->setEnabled(true);
}
void ScoringCalculationPage::customPointRemoveClicked()
{
	Ruleset* ruleset = getCurrentRuleset();
	int cur_index = customPointList->currentRow();
	
	delete customPointList->item(cur_index);
	ruleset->pointTableSettings.table.erase(ruleset->pointTableSettings.table.begin() + cur_index);
	
	if (customPointList->count() <= 1)
		customPointRemove->setEnabled(false);
}

void PointListWidget::dropEvent(QDropEvent* event)
{
    //QListWidget::dropEvent(event);
	QListView::dropEvent(event);

	QTimer::singleShot(20, this, SLOT(updateList()));
}
void PointListWidget::updateList()
{
	Ruleset* ruleset = page->getCurrentRuleset();
	std::vector<FPNumber>* v = &ruleset->pointTableSettings.table;
	
	v->clear();
	for (int i = 0; i < page->customPointList->count(); ++i)
		v->push_back(FPNumber(page->customPointList->item(i)->text().toDouble()));
}

void ScoringCalculationPage::customPointItemChanged(QListWidgetItem* item)
{
	if (!react_to_changes) return;
	
	Ruleset* ruleset = getCurrentRuleset();
	FPNumber number = FPNumber(item->text().toDouble());
	ruleset->pointTableSettings.table[customPointList->row(item)] = number;
	
	react_to_changes = false;
	item->setText(number.toString());
	react_to_changes = true;
}

void ScoringCalculationPage::timePointAddClicked()
{
	Ruleset* ruleset = getCurrentRuleset();
	int cur_row = timePointTable->currentRow();
	
	ruleset->timePointSettings.table.insert(ruleset->timePointSettings.table.begin() + (cur_row + 1), std::make_pair(FPNumber(100.0), FPNumber(25.0)));
	
	timePointTable->insertRow(cur_row + 1);
	timePointUpdateRow(cur_row + 1);
	timePointCellChanged(cur_row + 1, 0);	// put a (probably) valid default value there
	timePointTable->editItem(timePointTable->item(cur_row + 1, 0));
	
	timePointRemove->setEnabled(true);
}
void ScoringCalculationPage::timePointUpdateRow(int row)
{
	Ruleset* ruleset = getCurrentRuleset();
	
	react_to_changes = false;
	
	QTableWidgetItem* item = new QTableWidgetItem(ruleset->timePointSettings.table[row].first.toString());
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | ((row > 0) ? Qt::ItemIsEditable : Qt::NoItemFlags));
	timePointTable->setItem(row, 0, item);
	
	item = new QTableWidgetItem(ruleset->timePointSettings.table[row].second.toString());
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
	timePointTable->setItem(row, 1, item);
	
	react_to_changes = true;
}
void ScoringCalculationPage::timePointRemoveClicked()
{
	Ruleset* ruleset = getCurrentRuleset();
	int cur_row = timePointTable->currentRow();
	assert(cur_row != 0);
	
	timePointTable->removeRow(cur_row);
	ruleset->timePointSettings.table.erase(ruleset->timePointSettings.table.begin() + cur_row);
}
void ScoringCalculationPage::timePointCellChanged(int row, int column)
{
	if (!react_to_changes) return;
	
	Ruleset* ruleset = getCurrentRuleset();
	QTableWidgetItem* item = timePointTable->item(row, column);
	
	FPNumber number = FPNumber(item->text().toDouble());
	if (column == 0)
	{
		if (row > 0 && number.toInt() <= ruleset->timePointSettings.table[row - 1].first.toInt())
			number = FPNumber(ruleset->timePointSettings.table[row - 1].first.toInt() + 100);
		else if (row < (int)ruleset->timePointSettings.table.size() - 1 && number.toInt() >= ruleset->timePointSettings.table[row + 1].first.toInt())
			number = FPNumber(ruleset->timePointSettings.table[row + 1].first.toInt() - 100);
		
		ruleset->timePointSettings.table[row].first = number;
	}
	else if (column == 1)
		ruleset->timePointSettings.table[row].second = number;
	
	react_to_changes = false;
	item->setText(number.toString());
	react_to_changes = true;
}
void ScoringCalculationPage::timePointCurrentCellChanged(int current_row, int /*current_column*/, int /*previous_row*/, int /*previous_column*/)
{
	timePointRemove->setEnabled(current_row != 0);
}

void ScoringCalculationPage::setCustomCategories(bool enable)
{
	rulesetGroup->setVisible(enable);
	
	if (enable == false)
	{
		// Show standard ruleset
		rulesetList->setCurrentRow(0);
	}
}
void ScoringCalculationPage::rulesetChanged(QListWidgetItem* current, QListWidgetItem* /*previous*/)
{
	Ruleset* ruleset = static_cast<Ruleset*>(current->data(Qt::UserRole).value<void*>());
	react_to_changes = false;
	
	deleteRulesetButton->setEnabled(ruleset != scoring->getStandardRuleset());
	
	int id = static_cast<int>(ruleset->rule_type);
	calculationButtonGroup->button(id)->setChecked(true);
	settingsStack->setCurrentIndex(id);
	
	ratioButtonGroup->button(ruleset->timeRatioSettings.formulaNumber)->setChecked(true);
	ratio_1_factor->setText(ruleset->timeRatioSettings.formula1Factor.toString());
	ratio_2_factor->setText(ruleset->timeRatioSettings.formula2Factor.toString());
	ratio_3_bias->setText(ruleset->timeRatioSettings.formula3Bias.toString());
	ratio_3_factor->setText(ruleset->timeRatioSettings.formula3Factor.toString());
	ratio_3_average_percentage->setText(ruleset->timeRatioSettings.formula3AveragePercentage.toString());
	
	intervalEdit->setText(ruleset->fixedIntervalSettings.interval.toString());
	lastRunnerPointsEdit->setText(ruleset->fixedIntervalSettings.lastRunnerPoints.toString());
	countingRunnersPerClubEdit->setText(QString::number(ruleset->fixedIntervalSettings.countingRunnersPerClub));
	winnerPointButtonGroup->button(ruleset->fixedIntervalSettings.countTeams ? 1 : 0)->setChecked(true);
	disqualifiedRunnersCountCheck->setChecked(ruleset->fixedIntervalSettings.disqualifiedRunnersCount);
	
	customPointList->clear();
	int size = ruleset->pointTableSettings.table.size();
	for (int i = 0; i < size; ++i)
	{
		QListWidgetItem* new_item = new QListWidgetItem(ruleset->pointTableSettings.table[i].toString());
		new_item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
		customPointList->addItem(new_item);
	}
	customPointRemove->setEnabled(size > 1);
	if (size > 0)
		customPointList->setCurrentRow(0);
	
	timePointTable->clearContents();
	size = ruleset->timePointSettings.table.size();
	for (int i = 0; i < size; ++i)
	{
		timePointTable->insertRow(i);
		timePointUpdateRow(i);
	}
	timePointRemove->setEnabled(false);
	if (size > 0)
		timePointTable->setCurrentCell(0, 0);
	
	handicapCheck->setChecked(ruleset->handicapping);
	handicapGroup->setVisible(ruleset->handicapping);
	handicapList->clear();
	size = ruleset->handicaps.size();
	for (int i = 0; i < size; ++i)
	{
		Ruleset::HandicapSetting* handicap = ruleset->handicaps[i];
		QListWidgetItem* new_item = new QListWidgetItem(handicap->getDesc());
		new_item->setData(Qt::UserRole, qVariantFromValue<void*>(handicap));
		handicapList->addItem(new_item);
	}
	handicapList->sortItems();
	if (size > 0)
		handicapList->setCurrentRow(0);
	updateHandicapWidgets();
	
	react_to_changes = true;
}
void ScoringCalculationPage::updateHandicapWidgets()
{
	bool enable = handicapList->currentRow() != -1;
	
	handicapGenderCombo->setEnabled(enable);
	handicapStart->setEnabled(enable);
	handicapEnd->setEnabled(enable);
	handicapFactor->setEnabled(enable);
	handicapRemove->setEnabled(enable);
}
void ScoringCalculationPage::addRulesetClicked()
{
	Ruleset* new_ruleset = scoring->addRuleset();
	react_to_changes = false;
	
	QListWidgetItem* item = new QListWidgetItem(rulesetList);
	item->setText(new_ruleset->name);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled);
	item->setData(Qt::UserRole, qVariantFromValue<void*>(new_ruleset));
	
	react_to_changes = true;
	
	rulesetList->addItem(item);
	rulesetList->setCurrentItem(item);
	rulesetList->editItem(item);
	
	dialog->rulesetsChanged();
}
void ScoringCalculationPage::deleteRulesetClicked()
{
	Ruleset* ruleset = getCurrentRuleset();
	assert(ruleset != scoring->getStandardRuleset());
	
	// Check if this ruleset is used by a category
	int n = scoring->getNumCustomCategories();
	for (int i = 0; i < n; ++i)
	{
		CustomCategory* cat = scoring->getCustomCategory(i);
		if (cat->ruleset == ruleset)
		{
			// TODO: offer to do the described
			QMessageBox::warning(this, APP_NAME, tr("The ruleset %1 you want to delete is still used by at least one custom category. Please remove all references to this ruleset first!").arg(ruleset->name));
			return;
		}
	}
	
	if (QMessageBox::question(this, APP_NAME, tr("Confirm: delete the ruleset %1?").arg(ruleset->name), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
		return;

	scoring->deleteRuleset(ruleset);
	delete rulesetList->currentItem();
	
	dialog->rulesetsChanged();
}
void ScoringCalculationPage::rulesetRenamed(QListWidgetItem* item)
{
	if (!react_to_changes)	return;
	Ruleset* ruleset = static_cast<Ruleset*>(item->data(Qt::UserRole).value<void*>());
	ruleset->name = item->text();
	
	dialog->rulesetsChanged();
}

void ScoringCalculationPage::ruleTypeChanged(bool /*checked*/)
{
	if (!react_to_changes)	return;
	
	int id = calculationButtonGroup->checkedId();
	
	settingsStack->setCurrentIndex(id);
	getCurrentRuleset()->rule_type = static_cast<RuleType>(id);
}

void ScoringCalculationPage::formulaNumberChanged(int /*i*/)
{
	if (!react_to_changes)	return;
	getCurrentRuleset()->timeRatioSettings.formulaNumber = ratioButtonGroup->checkedId();
}
void ScoringCalculationPage::ratio1Changed()
{
	if (!react_to_changes)	return;
	getCurrentRuleset()->timeRatioSettings.formula1Factor = ratio_1_factor->text().toDouble();
}
void ScoringCalculationPage::ratio2Changed()
{
	if (!react_to_changes)	return;
	getCurrentRuleset()->timeRatioSettings.formula2Factor = ratio_2_factor->text().toDouble();
}
void ScoringCalculationPage::ratio3Changed()
{
	if (!react_to_changes)	return;
	getCurrentRuleset()->timeRatioSettings.formula3Bias = ratio_3_bias->text().toDouble();
	getCurrentRuleset()->timeRatioSettings.formula3Factor = ratio_3_factor->text().toDouble();
	getCurrentRuleset()->timeRatioSettings.formula3AveragePercentage = ratio_3_average_percentage->text().toDouble();
}

void ScoringCalculationPage::countingRunnersPerClubChanged()
{
	if (!react_to_changes)	return;
	getCurrentRuleset()->fixedIntervalSettings.countingRunnersPerClub = countingRunnersPerClubEdit->text().toInt();
}
void ScoringCalculationPage::disqualifiedRunnersCountToggled(bool checked)
{
	if (!react_to_changes)	return;
	getCurrentRuleset()->fixedIntervalSettings.disqualifiedRunnersCount = checked;
}
void ScoringCalculationPage::winnerPointCalculationChanged(int id)
{
	if (!react_to_changes)	return;
	getCurrentRuleset()->fixedIntervalSettings.countTeams = (id == 1);
}
void ScoringCalculationPage::intervalChanged()
{
	if (!react_to_changes)	return;
	getCurrentRuleset()->fixedIntervalSettings.interval = intervalEdit->text().toDouble();
}
void ScoringCalculationPage::lastRunnerPointsChanged()
{
	if (!react_to_changes)	return;
	getCurrentRuleset()->fixedIntervalSettings.lastRunnerPoints = lastRunnerPointsEdit->text().toDouble();
}

Ruleset* ScoringCalculationPage::getCurrentRuleset()
{
	return static_cast<Ruleset*>(rulesetList->currentItem()->data(Qt::UserRole).value<void*>());
}

// ### LimitRunnersPage ###

LimitRunnersPage::LimitRunnersPage(Scoring* _scoring, ScoringDialog* _dialog, QWidget* parent) : QWidget(parent), scoring(_scoring), dialog(_dialog)
{
	// Widgets
	limitRegionsCheck = new QCheckBox(tr("Limit to these regions:"));
	
	locationWidget = new QTreeWidget();
	locationWidget->header()->hide();
	QSortFilterProxyModel* model = locationDB.getSortModel();
	
	int i = 0;
	QModelIndex index = model->index(i, 0);
	while (index != QModelIndex())
	{
		QTreeWidgetItem* item = new QTreeWidgetItem(locationWidget);
		item->setText(0, model->data(index).toString());
		Location* location = locationDB.getItem(model->mapToSource(index));
		item->setData(0, Qt::UserRole, qVariantFromValue<void*>(location));
		
		int s = 0;
		bool has_checked = false;
		QModelIndex subIndex = model->index(s, 0, index);
		while (subIndex != QModelIndex())
		{
			QTreeWidgetItem* subItem = new QTreeWidgetItem(item);
			Location* subLocation = locationDB.getItem(model->mapToSource(subIndex));
			bool checked = scoring->isAllowedRegion(subLocation);
			has_checked |= checked;
			
			subItem->setText(0, model->data(subIndex).toString());
			subItem->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
			subItem->setData(0, Qt::UserRole, qVariantFromValue<void*>(subLocation));
			
			++s;
			subIndex = model->index(s, 0, index);
		}
		
		if (scoring->isAllowedRegion(location))
			item->setCheckState(0, Qt::Checked);
		else if (has_checked)
			item->setCheckState(0, Qt::PartiallyChecked);
		else
			item->setCheckState(0, Qt::Unchecked);
		
		++i;
		index = model->index(i, 0);
	};
	
	limitRegionsCheck->setCheckState(scoring->getLimitRegions() ? Qt::Checked : Qt::Unchecked);
	locationWidget->setEnabled(scoring->getLimitRegions());
	
	limitClubsCheck = new QCheckBox(tr("Limit to these clubs:"));
	
	clubWidget = new QListWidget();
	QSortFilterProxyModel* clubSortedModel = new QSortFilterProxyModel();
	model = clubSortedModel;
	clubSortedModel->setSourceModel(&clubDB);
	clubSortedModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	clubSortedModel->sort(0, Qt::AscendingOrder);
	
	i = 0;
	index = model->index(i, 0);
	while (index != QModelIndex())
	{
		QListWidgetItem* item = new QListWidgetItem(clubWidget);
		Club* club = clubDB.getItem(clubSortedModel->mapToSource(index));
		item->setText(model->data(index).toString());
		item->setCheckState(scoring->isAllowedClub(club) ? Qt::Checked : Qt::Unchecked);
		item->setData(Qt::UserRole, qVariantFromValue<void*>(club));
		
		++i;
		index = model->index(i, 0);
	};
	
	delete clubSortedModel;
	limitClubsCheck->setCheckState(scoring->getLimitClubs() ? Qt::Checked : Qt::Unchecked);
	clubWidget->setEnabled(scoring->getLimitClubs());
	
	// Layout
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(limitRegionsCheck);
	layout->addWidget(locationWidget);
	layout->addWidget(limitClubsCheck);
	layout->addWidget(clubWidget);
	setLayout(layout);
	
	// Connections
	connect(limitRegionsCheck, SIGNAL(toggled(bool)), this, SLOT(limitRegionsChanged(bool)));
	connect(locationWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(locationChecked(QTreeWidgetItem*,int)));
	connect(limitClubsCheck, SIGNAL(toggled(bool)), this, SLOT(limitClubsChanged(bool)));
	connect(clubWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(clubChecked(QListWidgetItem*)));
}

LimitRunnersPage::~LimitRunnersPage()
{
}

void LimitRunnersPage::limitClubsChanged(bool checked)
{
	clubWidget->setEnabled(checked);
	scoring->setLimitClubs(checked);
}
void LimitRunnersPage::locationChecked(QTreeWidgetItem* item, int column)
{
	if (column != 0)
		return;
	
	disconnect(locationWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(locationChecked(QTreeWidgetItem*,int)));
	
	if (item->parent() == nullptr)
	{
		// Top-level changed, update children
		Qt::CheckState state = item->checkState(0);
		bool checked = state == Qt::Checked;
		
		scoring->setAllowedRegion(static_cast<Location*>(item->data(0, Qt::UserRole).value<void*>()), checked);
		
		int childCount = item->childCount();
		for (int i = 0; i < childCount; ++i)
		{
			QTreeWidgetItem* child = item->child(i);
			scoring->setAllowedRegion(static_cast<Location*>(child->data(0, Qt::UserRole).value<void*>()), checked);
			
			if (child->checkState(0) != state)
				child->setCheckState(0, state);
		}
	}
	else
	{
		// Child changed, update top-level
		Qt::CheckState state = item->checkState(0);
		bool checked = state == Qt::Checked;
		
		scoring->setAllowedRegion(static_cast<Location*>(item->data(0, Qt::UserRole).value<void*>()), checked);
		
		int childCount = item->parent()->childCount();
		int i = 0;
		for (; i < childCount; ++i)
		{
			QTreeWidgetItem* checkIndex = item->parent()->child(i);
			if (checkIndex->checkState(0) != state)
				break;
		}
		
		if (i == childCount)
		{
			if (state == Qt::Checked && item->parent()->checkState(0) != Qt::Checked)
				scoring->setAllowedRegion(static_cast<Location*>(item->parent()->data(0, Qt::UserRole).value<void*>()), true);
			item->parent()->setCheckState(0, state);
		}
		else
		{
			if (state == Qt::Unchecked && item->parent()->checkState(0) == Qt::Checked)
				scoring->setAllowedRegion(static_cast<Location*>(item->parent()->data(0, Qt::UserRole).value<void*>()), false);
			item->parent()->setCheckState(0, Qt::PartiallyChecked);
		}
	}
	
	connect(locationWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(locationChecked(QTreeWidgetItem*,int)));
}
void LimitRunnersPage::clubChecked(QListWidgetItem* item)
{
	scoring->setAllowedClub(static_cast<Club*>(item->data(Qt::UserRole).value<void*>()), item->checkState() == Qt::Checked);
}
void LimitRunnersPage::limitRegionsChanged(bool checked)
{
	locationWidget->setEnabled(checked);
	scoring->setLimitRegions(checked);
}

// ### AdjustCategoriesPage ###

AdjustCategoriesPage::AdjustCategoriesPage(Scoring* _scoring, ScoringDialog* _dialog, QWidget* parent) : QWidget(parent), scoring(_scoring), dialog(_dialog)
{
	react_to_changes = true;
	
	QGroupBox* categoriesGroup = new QGroupBox(tr("Custom categories"));
	categoriesList = new QListWidget();
	QPushButton* addCategoryButton = new QPushButton(QIcon("images/plus.png"), "");
	deleteCategoryButton = new QPushButton(QIcon("images/minus.png"), "");
	
	QGridLayout* categoryLayout = new QGridLayout();
	categoryLayout->addWidget(categoriesList, 0, 0, 3, 1);
	categoryLayout->setRowStretch(2, 1);
	categoryLayout->addWidget(addCategoryButton, 0, 1);
	categoryLayout->addWidget(deleteCategoryButton, 1, 1);
	categoriesGroup->setLayout(categoryLayout);
	
	QGroupBox* settingsGroup = new QGroupBox(tr("Category settings"));
	
	QLabel* nameLabel = new QLabel(tr("Name:"));
	nameEdit = new QLineEdit();
	QHBoxLayout* nameLayout = new QHBoxLayout();
	nameLayout->addWidget(nameLabel);
	nameLayout->addWidget(nameEdit);
	
	QLabel* sourceLabel = new QLabel(tr("From categories:"));
	sourceEdit = new QLineEdit();
	QHBoxLayout* sourceLayout = new QHBoxLayout();
	sourceLayout->addWidget(sourceLabel);
	sourceLayout->addWidget(sourceEdit);
	
	QLabel* rulesetLabel = new QLabel(tr("Uses ruleset:"));
	rulesetCombo = new QComboBox();
	QHBoxLayout* rulesetLayout = new QHBoxLayout();
	rulesetLayout->addWidget(rulesetLabel);
	rulesetLayout->addWidget(rulesetCombo);
	
	QVBoxLayout* settingsLayout = new QVBoxLayout();
	settingsLayout->addLayout(nameLayout);
	settingsLayout->addLayout(sourceLayout);
	settingsLayout->addLayout(rulesetLayout);
	settingsGroup->setLayout(settingsLayout);
	
	
	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->addWidget(categoriesGroup);
	mainLayout->addWidget(settingsGroup);
	setLayout(mainLayout);
	
	// Set data
	int size = scoring->getNumCustomCategories();
	for (int i = 0; i < size; ++i)
	{
		CustomCategory* cat = scoring->getCustomCategory(i);
		
		QListWidgetItem* item = new QListWidgetItem(categoriesList);
		item->setText(cat->name);
		item->setData(Qt::UserRole, qVariantFromValue<void*>(cat));
	}
	
	updateRulesetCombo(false);
	
	nameEdit->setEnabled(false);
	sourceEdit->setEnabled(false);
	rulesetCombo->setEnabled(false);
	deleteCategoryButton->setEnabled(false);
	
	connect(addCategoryButton, SIGNAL(clicked()), this, SLOT(addCategoryClicked()));
	connect(deleteCategoryButton, SIGNAL(clicked()), this, SLOT(deleteCategoryClicked()));
	connect(categoriesList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(categoryChanged(QListWidgetItem*,QListWidgetItem*)));
	
	connect(nameEdit, SIGNAL(textEdited(QString)), this, SLOT(nameChanged(QString)));
	connect(sourceEdit, SIGNAL(editingFinished()), this, SLOT(sourcesChanged()));
	connect(rulesetCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(rulesetChanged(int)));
}
AdjustCategoriesPage::~AdjustCategoriesPage()
{
}

void AdjustCategoriesPage::addCategoryClicked()
{
	CustomCategory* new_category = scoring->addCustomCategory();
	react_to_changes = false;
	
	QListWidgetItem* item = new QListWidgetItem(categoriesList);
	item->setText(new_category->name);
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	item->setData(Qt::UserRole, qVariantFromValue<void*>(new_category));
	
	react_to_changes = true;
	
	categoriesList->addItem(item);
	categoriesList->setCurrentItem(item);
	//rulesetList->editItem(item);
}
void AdjustCategoriesPage::deleteCategoryClicked()
{
	CustomCategory* cat = getCurrentCategory();
	assert(cat);
	
	scoring->deleteCustomCategory(cat);
	delete categoriesList->currentItem();
}
void AdjustCategoriesPage::categoryChanged(QListWidgetItem* /*current*/, QListWidgetItem* /*previous*/)
{
	react_to_changes = false;
	
	CustomCategory* cat = getCurrentCategory();
	bool enable = cat != nullptr;
	
	nameEdit->setEnabled(enable);
	sourceEdit->setEnabled(enable);
	rulesetCombo->setEnabled(enable);
	deleteCategoryButton->setEnabled(enable);
	
	if (cat)
	{
		nameEdit->setText(cat->name);
		sourceEdit->setText(cat->sourceCategories.join(", "));
		int index = rulesetCombo->findData(qVariantFromValue<void*>(cat->ruleset), Qt::UserRole);
		assert(index != -1);
		rulesetCombo->setCurrentIndex(index);
	}
	
	react_to_changes = true;
}

void AdjustCategoriesPage::nameChanged(QString text)
{
	if (!react_to_changes) return;
	CustomCategory* cat = getCurrentCategory();
	assert(cat);
	
	cat->name = text;
	categoriesList->item(categoriesList->currentRow())->setText(text);
}
void AdjustCategoriesPage::sourcesChanged()
{
	if (!react_to_changes) return;
	CustomCategory* cat = getCurrentCategory();
	assert(cat);
	
	cat->sourceCategories = sourceEdit->text().split(",", QString::SkipEmptyParts);
	for (int i = 0; i < cat->sourceCategories.size(); ++i)
	{
		cat->sourceCategories[i] = cat->sourceCategories[i].trimmed();
		if (cat->sourceCategories[i].isEmpty())
		{
			cat->sourceCategories.removeAt(i);
			--i;
		}
	}
}
void AdjustCategoriesPage::rulesetChanged(int /*index*/)
{
	if (!react_to_changes) return;
	CustomCategory* cat = getCurrentCategory();
	assert(cat);
	
	cat->ruleset = static_cast<Ruleset*>(rulesetCombo->itemData(rulesetCombo->currentIndex()).value<void*>());
}

CustomCategory* AdjustCategoriesPage::getCurrentCategory()
{
	if (categoriesList->currentItem() == nullptr)
		return nullptr;
	
	return static_cast<CustomCategory*>(categoriesList->currentItem()->data(Qt::UserRole).value<void*>());
}
void AdjustCategoriesPage::updateRulesetCombo(bool keepSelection)
{
	disconnect(rulesetCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(rulesetChanged(int)));
	
	Ruleset* selection = nullptr;
	if (keepSelection && rulesetCombo->currentIndex() != -1)
		selection = static_cast<Ruleset*>(rulesetCombo->itemData(rulesetCombo->currentIndex(), Qt::UserRole).value<void*>());
	
	rulesetCombo->clear();
	
	int size = scoring->getNumRulesets();
	for (int i = 0; i < size; ++i)
	{
		Ruleset* ruleset = scoring->getRuleset(i);
		rulesetCombo->addItem(ruleset->name, qVariantFromValue<void*>(ruleset));
		
		if (selection == ruleset)
			rulesetCombo->setCurrentIndex(rulesetCombo->count() - 1);
	}
	
	connect(rulesetCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(rulesetChanged(int)));
}

// ### TeamScoringPage ###

TeamScoringPage::TeamScoringPage(Scoring* _scoring, ScoringDialog* _dialog, QWidget* parent) : QWidget(parent), scoring(_scoring), dialog(_dialog)
{
	QLabel* excludeLabel = new QLabel(tr("Categories to exclude from team scoring:"));
	excludeEdit = new QLineEdit();
	
	QHBoxLayout* excludeLayout = new QHBoxLayout();
	excludeLayout->addWidget(excludeLabel);
	excludeLayout->addWidget(excludeEdit);
	
	showSingleResultsCheck = new QCheckBox(tr("Show single results in result listing by category"));
	showSingleResultsCheck->setToolTip(tr("When more than one result contributes to the point sum of a team in a specific category, this option shows all runner results one-by-one in the \"results by category\" list, otherwise only the sum of the results will be shown there."));
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(excludeLayout);
	layout->addWidget(showSingleResultsCheck);
	layout->addStretch(1);
	setLayout(layout);
	
	// Set values
	excludeEdit->setText(scoring->getTeamExcludeCategories().join(", "));
	showSingleResultsCheck->setChecked(scoring->getShowSingleResultsInCategoryListing());
	
	connect(excludeEdit, SIGNAL(editingFinished()), this, SLOT(excludeChanged()));
	connect(showSingleResultsCheck, SIGNAL(clicked(bool)), this, SLOT(showSingleResultsChecked(bool)));
}
TeamScoringPage::~TeamScoringPage()
{
}

void TeamScoringPage::excludeChanged()
{
	QStringList& list = scoring->getTeamExcludeCategories();
	
	list = excludeEdit->text().split(",", QString::SkipEmptyParts);
	for (int i = 0; i < list.size(); ++i)
	{
		list[i] = list[i].trimmed();
		if (list[i].isEmpty())
		{
			list.removeAt(i);
			--i;
		}
	}
}
void TeamScoringPage::showSingleResultsChecked(bool checked)
{
	scoring->setShowSingleResultsInCategoryListing(checked);
}

// ### EmptyPage ###

EmptyPage::EmptyPage(Scoring* _scoring, ScoringDialog* _dialog, QWidget* parent) : QWidget(parent), scoring(_scoring), dialog(_dialog)
{
}
