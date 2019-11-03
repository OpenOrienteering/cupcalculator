/*
 *    Copyright 2011 Thomas Schöps
 *    
 *    This file is part of OpenOrienteering's scoring tool.
 * 
 *    OpenOrienteering's scoring tool is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 * 
 *    OpenOrienteering's scoring tool is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 * 
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering's scoring tool.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "test.h"

#include <algorithm>
#include <utility>
#include <vector>

#include <QtTest>
#include <QByteArray>
#include <QGuiApplication>
#include <QList>
#include <QStringList>
#include <QVariant>

#include "club.h"
#include "csvFile.h"
#include "event.h"
#include "mainWindow.h"
#include "resultList.h"
#include "scoring.h"
#include "util.h"

void TestScoringTool::fixedPointCalculation()
{
	// Import test data
	QString input = "Nachname;Vorname;Jg;G;AK;Wertung;Abk;Ort;Lang;Zeit\n"
					// 3 Clubs
					"Mr;AAA;80;M;x;0;Club;A;Category A;1:00\n"				// AK
					"Mr;AAB;80;M;0;0;Club;NotInScoring;Category A;1:00\n"	// Not in scoring
					"Mr;AAC;80;M;0;0;Club;A;Category A;1:00\n"				// 8
					"Mr;AAD;80;M;0;0;Club;B;Category A;1:00\n"				// 8
					"Mr;AAE;80;M;0;0;Club;B;Category A;1:00\n"				// 8
					"Mr;AAF;80;M;0;3;Club;C;Category A;1:00\n"				// MP
				
					"Mr;BBB;80;M;0;0;Club;NotInScoring;Category A;2:00\n"	// Not in scoring
					"Mr;BBC;80;M;0;0;Club;B;Category A;2:00\n"				// 0
					"Mr;BBD;80;M;0;0;Club;A;Category A;2:00\n";				// 5
	
	CSVFile file(&input);
	if (!file.open(true))
		QFAIL("Cannot read input from string");
	
	MainWindow* window = new MainWindow();
	if (!window->importResultsFromCSV(&file, false))
		QFAIL("Cannot import input data");
	
	// Setup scoring
	Scoring* scoring = new Scoring("Test Scoring");
	Ruleset* ruleset = scoring->getRuleset(0);
	
	scoring->setDecimalPlaces(0);
	
	ruleset->rule_type = FixedInterval;
	ruleset->fixedIntervalSettings.countingRunnersPerClub = 2;
	ruleset->fixedIntervalSettings.countTeams = true;
	ruleset->fixedIntervalSettings.disqualifiedRunnersCount = true;
	ruleset->fixedIntervalSettings.interval = 1;
	ruleset->fixedIntervalSettings.lastRunnerPoints = 3;
	
	scoring->setLimitRunners(true);
	scoring->setLimitClubs(true);
	
	Club* club = clubDB.findClub("Club B");
	QVERIFY(club != nullptr);
	scoring->setAllowedClub(club, true);
	QVERIFY(scoring->isAllowedClub(club));
	club = clubDB.findClub("Club A");
	QVERIFY(club != nullptr);
	scoring->setAllowedClub(club, true);
	QVERIFY(scoring->isAllowedClub(club));
	club = clubDB.findClub("Club C");
	QVERIFY(club != nullptr);
	scoring->setAllowedClub(club, true);
	QVERIFY(scoring->isAllowedClub(club));
	
	// Calculate results
	std::vector<ResultList*> results;
	scoring->calculateScoring(window->getEvent()->getResultList(), 2000, results, Scoring::IncludeAllRunners);

	// Check results
	QString output;
	CSVFile out_file(&output);
	if (!out_file.open(false))
		QFAIL("Cannot open output string");
	results[0]->exportToCSV(&out_file);
	out_file.close();
	
	QString oracle = "Category;Rank;First name;Surname;Club;Time;Points\n"
						 "Category A;1;AAD;Mr;Club B;1:00;8\n"
						 "Category A;1;AAE;Mr;Club B;1:00;8\n"
						 "Category A;-;AAA;Mr;Club A;1:00;-\n"
						 "Category A;-;AAB;Mr;Club NotInScoring;1:00;-\n"
						 "Category A;1;AAC;Mr;Club A;1:00;8\n"
						 "Category A;4;BBD;Mr;Club A;2:00;5\n"
						 "Category A;-;BBB;Mr;Club NotInScoring;2:00;-\n"
						 "Category A;4;BBC;Mr;Club B;2:00;0\n"
						 "Category A;-;AAF;Mr;Club C;1:00;-\n";
	checkResults(results[0], oracle);
}

void TestScoringTool::timeRatioCalculation()
{
	if (QGuiApplication::platformName() == "offscreen")
		QSKIP("cannot show dialogs on 'offscreen' platform");
	
	// Import test data
	QString input = "Nachname;Vorname;Jg;G;AK;Wertung;Abk;Ort;Lang;Zeit\n"
					// 3 Clubs
					"Mr;AAA;80;M;x;0;Club;A;Category A;1:00\n"				// AK
					"Mr;AAB;80;M;0;0;Club;NotInScoring;Category A;1:00\n"	// Not in scoring
					"Mr;AAC;80;M;0;0;Club;A;Category A;1:00\n"				// 100
					"Mr;AAD;80;M;0;0;Club;B;Category A;1:00\n"				// 100
					"Mr;AAE;80;M;0;0;Club;B;Category A;1:00\n"				// 100
					"Mr;AAF;80;M;0;3;Club;C;Category A;1:00\n"				// MP
				
					"Mr;BBB;80;M;0;0;Club;NotInScoring;Category A;2:00\n"	// Not in scoring
					"Mr;BBC;80;M;0;0;Club;B;Category A;2:00\n"				// 50
					"Mr;BBD;80;M;0;0;Club;A;Category A;2:00\n"				// 50
					"Mr;XXA;80;M;x;0;Club;B;Category B;1:00\n"				// AK
					"Mr;XXB;80;M;0;3;Club;B;Category B;1:00\n"				// MP
					"Mr;XXC;80;M;0;0;Club;B;Category B;4:00\n"				// 100
					"Mr;XXD;80;M;0;0;Club;B;Category B;16:00\n";			// 25
	
	CSVFile file(&input);
	if (!file.open(true))
		QFAIL("Cannot read input from string");
	
	MainWindow* window = new MainWindow();
	if (!window->importResultsFromCSV(&file, false))
		QFAIL("Cannot import input data");
	
	// Setup scoring
	Scoring* scoring = new Scoring("Test Scoring");
	Ruleset* ruleset = scoring->getRuleset(0);
	
	scoring->setDecimalPlaces(0);
	
	ruleset->rule_type = TimeRatio;
	ruleset->timeRatioSettings.formula1Factor = 100;
	ruleset->timeRatioSettings.formulaNumber = 0;
	
	scoring->setLimitRunners(true);
	scoring->setLimitClubs(true);
	
	Club* club = clubDB.findClub("Club B");
	QVERIFY(club != nullptr);
	scoring->setAllowedClub(club, true);
	QVERIFY(scoring->isAllowedClub(club));
	club = clubDB.findClub("Club A");
	QVERIFY(club != nullptr);
	scoring->setAllowedClub(club, true);
	QVERIFY(scoring->isAllowedClub(club));
	club = clubDB.findClub("Club C");
	QVERIFY(club != nullptr);
	scoring->setAllowedClub(club, true);
	QVERIFY(scoring->isAllowedClub(club));
	
	// Calculate results
	std::vector<ResultList*> results;
	scoring->calculateScoring(window->getEvent()->getResultList(), 2000, results, Scoring::IncludeAllRunners);

	// Check results
	QString oracle = "Category;Rank;First name;Surname;Club;Time;Points\n"
						 "Category A;1;AAD;Mr;Club B;1:00;100\n"
						 "Category A;1;AAE;Mr;Club B;1:00;100\n"
						 "Category A;-;AAA;Mr;Club A;1:00;-\n"
						 "Category A;-;AAB;Mr;Club NotInScoring;1:00;-\n"
						 "Category A;1;AAC;Mr;Club A;1:00;100\n"
						 "Category A;4;BBD;Mr;Club A;2:00;50\n"
						 "Category A;-;BBB;Mr;Club NotInScoring;2:00;-\n"
						 "Category A;4;BBC;Mr;Club B;2:00;50\n"
						 "Category A;-;AAF;Mr;Club C;1:00;-\n"
						 
						 "Category B;-;XXA;Mr;Club B;1:00;-\n"
						 "Category B;-;XXB;Mr;Club B;1:00;-\n"
						 "Category B;1;XXC;Mr;Club B;4:00;100\n"
						 "Category B;2;XXD;Mr;Club B;16:00;25\n";
						 
						 //"Mr;XXA;80;M;x;0;Club;B;Category B;1:00\n"				// AK
						 //"Mr;XXB;80;M;0;3;Club;B;Category B;1:00\n"				// MP
						 //"Mr;XXC;80;M;0;0;Club;B;Category B;4:00\n"				// 100
						 //"Mr;XXD;80;M;0;0;Club;B;Category B;16:00\n";			// 25
	checkResults(results[0], oracle);
}

void TestScoringTool::timeRatioCalculationFormula3()
{
	if (QGuiApplication::platformName() == "offscreen")
		QSKIP("cannot show dialogs on 'offscreen' platform");
	
	// Import test data
	QString input = "Nachname;Vorname;Jg;G;AK;Wertung;Abk;Ort;Lang;Zeit\n"
	// 3 Clubs
	"Mr;AAA;80;M;x;0;Club;A;Category A;1:00\n"				// AK
	"Mr;AAB;80;M;0;0;Club;NotInScoring;Category A;1:00\n"	// Not in scoring
	"Mr;AAC;80;M;0;0;Club;A;Category A;1:00\n"				// 100
	"Mr;AAD;80;M;0;0;Club;B;Category A;1:00\n"				// 100
	"Mr;AAE;80;M;0;0;Club;B;Category A;1:00\n"				// 100
	"Mr;AAF;80;M;0;3;Club;C;Category A;1:00\n"				// MP
	
	"Mr;BBB;80;M;0;0;Club;NotInScoring;Category A;2:00\n"	// Not in scoring
	"Mr;BBC;80;M;0;0;Club;B;Category A;2:00\n"				// 50
	"Mr;BBD;80;M;0;0;Club;A;Category A;2:00\n"				// 50
	"Mr;XXA;80;M;x;0;Club;B;Category B;1:00\n"				// AK
	"Mr;XXB;80;M;0;3;Club;B;Category B;1:00\n"				// MP
	"Mr;XXC;80;M;0;0;Club;B;Category B;4:00\n"				// 100
	"Mr;XXD;80;M;0;0;Club;B;Category B;16:00\n";			// 25
	
	CSVFile file(&input);
	if (!file.open(true))
		QFAIL("Cannot read input from string");
	
	MainWindow* window = new MainWindow();
	if (!window->importResultsFromCSV(&file, false))
		QFAIL("Cannot import input data");
	
	// Setup scoring
		Scoring* scoring = new Scoring("Test Scoring");
		Ruleset* ruleset = scoring->getRuleset(0);
		
		scoring->setDecimalPlaces(0);
		
		ruleset->rule_type = TimeRatio;
		ruleset->timeRatioSettings.formula3Bias = 100;
		ruleset->timeRatioSettings.formula3Factor = 100;
		ruleset->timeRatioSettings.formula3AveragePercentage = 80;
		ruleset->timeRatioSettings.formulaNumber = 2;
		
		scoring->setLimitRunners(true);
		scoring->setLimitClubs(true);
		
		Club* club = clubDB.findClub("Club B");
		QVERIFY(club != nullptr);
		scoring->setAllowedClub(club, true);
		QVERIFY(scoring->isAllowedClub(club));
		club = clubDB.findClub("Club A");
		QVERIFY(club != nullptr);
		scoring->setAllowedClub(club, true);
		QVERIFY(scoring->isAllowedClub(club));
		club = clubDB.findClub("Club C");
		QVERIFY(club != nullptr);
		scoring->setAllowedClub(club, true);
		QVERIFY(scoring->isAllowedClub(club));
		
		// Calculate results
		std::vector<ResultList*> results;
		scoring->calculateScoring(window->getEvent()->getResultList(), 2000, results, Scoring::IncludeAllRunners);
		
		// Check results
		QString oracle = "Category;Rank;First name;Surname;Club;Time;Points\n"
		"Category A;1;AAD;Mr;Club B;1:00;129\n"
		"Category A;1;AAE;Mr;Club B;1:00;129\n"
		"Category A;-;AAA;Mr;Club A;1:00;-\n"
		"Category A;-;AAB;Mr;Club NotInScoring;1:00;-\n"
		"Category A;1;AAC;Mr;Club A;1:00;129\n"
		"Category A;4;BBD;Mr;Club A;2:00;57\n"
		"Category A;-;BBB;Mr;Club NotInScoring;2:00;-\n"
		"Category A;4;BBC;Mr;Club B;2:00;57\n"
		"Category A;-;AAF;Mr;Club C;1:00;-\n"
		
		"Category B;-;XXA;Mr;Club B;1:00;-\n"
		"Category B;-;XXB;Mr;Club B;1:00;-\n"
		"Category B;1;XXC;Mr;Club B;4:00;160\n"
		"Category B;2;XXD;Mr;Club B;16:00;40\n";
		
		checkResults(results[0], oracle);
}

void TestScoringTool::timePointCalculation()
{
	if (QGuiApplication::platformName() == "offscreen")
		QSKIP("cannot show dialogs on 'offscreen' platform");
	
	// Import test data
	QString input = "Nachname;Vorname;Jg;G;AK;Wertung;Abk;Ort;Lang;Zeit\n"
					// 3 Clubs
					"Mr;AAA;80;M;x;0;Club;A;Category A;1:00\n"				// AK
					"Mr;AAB;80;M;0;0;Club;NotInScoring;Category A;1:00\n"	// Not in scoring
					"Mr;AAC;80;M;0;0;Club;A;Category A;1:00\n"				// 100
					"Mr;AAD;80;M;0;0;Club;B;Category A;1:00\n"				// 100
					"Mr;AAE;80;M;0;0;Club;B;Category A;1:00\n"				// 100
					"Mr;AAF;80;M;0;3;Club;C;Category A;1:00\n"				// MP
				
					"Mr;BBB;80;M;0;0;Club;NotInScoring;Category A;2:00\n"	// Not in scoring
					"Mr;BBC;80;M;0;0;Club;B;Category A;2:00\n"				// 50
					"Mr;BBD;80;M;0;0;Club;A;Category A;2:00\n"				// 50
					"Mr;XXA;80;M;x;0;Club;B;Category B;1:00\n"				// AK
					"Mr;XXB;80;M;0;3;Club;B;Category B;1:00\n"				// MP
					"Mr;XXC;80;M;0;0;Club;B;Category B;4:00\n"				// 100
					"Mr;XXD;80;M;0;0;Club;B;Category B;16:00\n"				// 25
					"Mr;XXE;80;M;0;0;Club;B;Category B;16:01\n";			// 0
	
	CSVFile file(&input);
	if (!file.open(true))
		QFAIL("Cannot read input from string");
	
	MainWindow* window = new MainWindow();
	if (!window->importResultsFromCSV(&file, false))
		QFAIL("Cannot import input data");
	
	// Setup scoring
	Scoring* scoring = new Scoring("Test Scoring");
	Ruleset* ruleset = scoring->getRuleset(0);
	
	scoring->setDecimalPlaces(0);
	
	ruleset->rule_type = TimePoints;
	ruleset->timePointSettings.table.clear();
	ruleset->timePointSettings.table.push_back(std::make_pair(FPNumber(100.0), FPNumber(100.0)));
	ruleset->timePointSettings.table.push_back(std::make_pair(FPNumber(200.0), FPNumber(50.0)));
	ruleset->timePointSettings.table.push_back(std::make_pair(FPNumber(400.0), FPNumber(25.0)));
	ruleset->timePointSettings.table.push_back(std::make_pair(FPNumber(400.01), FPNumber(0.0)));
	
	scoring->setLimitRunners(true);
	scoring->setLimitClubs(true);
	
	Club* club = clubDB.findClub("Club B");
	QVERIFY(club != nullptr);
	scoring->setAllowedClub(club, true);
	QVERIFY(scoring->isAllowedClub(club));
	club = clubDB.findClub("Club A");
	QVERIFY(club != nullptr);
	scoring->setAllowedClub(club, true);
	QVERIFY(scoring->isAllowedClub(club));
	club = clubDB.findClub("Club C");
	QVERIFY(club != nullptr);
	scoring->setAllowedClub(club, true);
	QVERIFY(scoring->isAllowedClub(club));
	
	// Calculate results
	std::vector<ResultList*> results;
	scoring->calculateScoring(window->getEvent()->getResultList(), 2000, results, Scoring::IncludeAllRunners);

	// Check results
	QString oracle = "Category;Rank;First name;Surname;Club;Time;Points\n"
						 "Category A;1;AAD;Mr;Club B;1:00;100\n"
						 "Category A;1;AAE;Mr;Club B;1:00;100\n"
						 "Category A;-;AAA;Mr;Club A;1:00;-\n"
						 "Category A;-;AAB;Mr;Club NotInScoring;1:00;-\n"
						 "Category A;1;AAC;Mr;Club A;1:00;100\n"
						 "Category A;4;BBD;Mr;Club A;2:00;50\n"
						 "Category A;-;BBB;Mr;Club NotInScoring;2:00;-\n"
						 "Category A;4;BBC;Mr;Club B;2:00;50\n"
						 "Category A;-;AAF;Mr;Club C;1:00;-\n"
						 
						 "Category B;-;XXA;Mr;Club B;1:00;-\n"
						 "Category B;-;XXB;Mr;Club B;1:00;-\n"
						 "Category B;1;XXC;Mr;Club B;4:00;100\n"
						 "Category B;2;XXD;Mr;Club B;16:00;25\n"
						 "Category B;3;XXE;Mr;Club B;16:01;0\n";
	checkResults(results[0], oracle);
}

void TestScoringTool::checkResults(ResultList* result_list, const QString& oracle)
{
	QString output;
	CSVFile out_file(&output);
	if (!out_file.open(false))
		QFAIL("Cannot open output string");
	result_list->exportToCSV(&out_file);
	out_file.close();
	
	QStringList output_list = output.split('\n');
	QStringList oracle_list = oracle.split('\n');
	
	// Check size of results
	QCOMPARE(output_list.size(), oracle_list.size());
	// Check that every result line is in the orcale string
	for (int i = 0; i < output_list.size(); ++i)
	{
		if (!oracle_list.contains(output_list[i]))
			QFAIL(("Wrong line in output:\n" + output_list[i]).toLatin1());
	}
	// Check ordering of results
	int prev_rank = -1;
	AbstractCategory* current_category = nullptr;
	for (int i = 0; i < result_list->rowCount(); ++i)
	{
		AbstractCategory* category = static_cast<AbstractCategory*>(result_list->getData(i, result_list->getCategoryColumn()).value<void*>());
		if (category != current_category)
		{
			prev_rank = -1;
			current_category = category;
		}
		
		QVariant new_rank_variant = result_list->getData(i, result_list->getRankColumn());
		if (new_rank_variant.isValid())
		{
			int new_rank = new_rank_variant.toInt();
			if (new_rank >= 0)
			{
				if (new_rank < prev_rank)
					QFAIL("Wrong ordering of results!");
				prev_rank = new_rank;
			}
		}
	}
	
	// This is not used because the ordering of competitors with the same time is undefined
	//QVERIFY(oracle.compare(output) == 0);
}

QTEST_MAIN(TestScoringTool)
