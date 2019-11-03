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


#ifndef SERIES_SCORING_H
#define SERIES_SCORING_H

#include <vector>
#include <map>
#include <set>

#include <QMap>

#include "util.h"
#include "problemWidget.h"

class AbstractCategory;
class Runner;
class ResultList;
class Club;

enum OrganizerBonus
{
	NoOrganizerBonus = 0,
	FixedOrganizerBonus = 1,
	PercentageOrganizerBonus = 2
};

/// Result of a single race in a series
struct SeriesRaceResult
{
	QString name;
	ResultList* list;
	std::set<Runner*> organizers;
};

/// Aggregates result lists from multiple races and calculates a series scoring, possibly also taking into account some organizer bonus
class SeriesScoring : public QObject
{
Q_OBJECT
public:
	
	typedef std::map< int, SeriesRaceResult* > ResultMap;
	
	/// Creates a new series scoring
	SeriesScoring(const QString& _fileName);
	~SeriesScoring();
	
	bool saveToFile(const QString path_prefix = "");
	bool loadFromFile(QWidget* dialogParent);
	
	void recalculateResult();
	
	/// Replace all ocurrences of club src with dest
	void mergeClubs(Club* src, Club* dest);
	
	/// Replace all ocurrences of runner src with dest
	void mergeRunners(Runner* src, Runner* dest);
	
	AbstractCategory* findInternCategory(const QString& name);
	
	inline ResultMap::iterator beginResults() {return resultMap.begin();}
	inline ResultMap::iterator endResults() {return resultMap.end();}
	/// Adds a result to the series. Creates a copy of the list internally.
	SeriesRaceResult* addResult(int run_number, const QString& name, ResultList* list, const std::set<Runner*>& organizers);
	void deleteResult(int run_number);
	void deleteResult(SeriesRaceResult* result);
	void changeResultNumber(int src, int dest);
	bool hasResult(int run_number);
	int getRaceResultNumber(SeriesRaceResult* result);
	
	bool referencesRunner(Runner* runner);
	
	inline void setCountOnlyBestRuns(const bool value) {count_only_best_runs = value; result_dirty = true;}
	inline bool getCountOnlyBestRuns() const {return count_only_best_runs;}
	inline void setNumCountingRuns(const int value) {num_counting_runs = value; result_dirty = true;}
	inline int getNumCountingRuns() const {return num_counting_runs;}
	
	inline void setOrganizerBonusType(const int value) {organizer_bonus_type = value; result_dirty = true;}
	inline int getOrganizerBonusType() const {return organizer_bonus_type;}
	inline void setOrganizerBonusFixed(const FPNumber& value) {organizer_bonus_fixed = value; result_dirty = true;}
	inline const FPNumber& getOrganizerBonusFixed() const {return organizer_bonus_fixed;}
	inline void setOrganizerBonusPercentage(const FPNumber& value) {organizer_bonus_percentage = value; result_dirty = true;}
	inline const FPNumber& getOrganizerBonusPercentage() const {return organizer_bonus_percentage;}
	inline void setOrganizerBonusCountingRuns(const int value) {organizer_bonus_counting_runs = value; result_dirty = true;}
	inline int getOrganizerBonusCountingRuns() const {return organizer_bonus_counting_runs;}
	
	inline ResultList* getResult() const {return result;}
	inline bool isResultDirty() const {return result_dirty;}
	inline void setResultDirty() {result_dirty = true;}
	
	inline void setFileName(const QString& value) {fileName = value;}
	inline const QString& getFileName() const {return fileName;}
	
private:
	
	void giveOrganizerBonus(int row, int column, int first_point_column, ResultList* result);
	AbstractCategory* findOrAddCategory(const QString& name, int number);
	void cleanupCategories();
	
	bool count_only_best_runs;
	int num_counting_runs;
	
	int organizer_bonus_type;	// from enum OrganizerBonus
	FPNumber organizer_bonus_fixed;
	FPNumber organizer_bonus_percentage;
	int organizer_bonus_counting_runs;
	
	ResultMap resultMap;		// maps race numbers -> race result lists
	ResultList* result;			// the end result
	bool result_dirty;
	
	std::vector< AbstractCategory* > categories;
	
	QString fileName;
};

/// Manages all series scorings which were ever used, loads only those that are necessary
class SeriesScoringDB
{
public:
	
	typedef QMap<QString, SeriesScoring*> Scorings;
	
	bool saveToFile(const QString path_prefix = "");
	
	/// Returns the scoring with the given name; loads it if it is not loaded yet.
	/// Returns nullptr if there was an error loading the scoring.
	SeriesScoring* getOrLoadScoring(const QString& name, QWidget* dialogParent);
	
	/// Appends #2, #3, ... in case the desiredName is already used until an unused name is found
	SeriesScoring* createNewScoring(const QString& desiredName);
	
	/// Does nothing if the old name does not exist
	void changeScoringName(const QString& old_name, const QString& new_name);
	
	/// Removes the scoring from the database and deletes the scoring file
	void deleteScoring(const QString& name);
	
	/// Checks if at least one open series scoring references the runner
	bool openScoringReferencesRunner(Runner* runner);
	
	/// Replace all ocurrences of club src with dest
	void mergeClubs(Club* src, Club* dest);
	
	/// Replace all ocurrences of runner src with dest
	void mergeRunners(Runner* src, Runner* dest);
	
	inline Scorings::iterator begin() {return scorings.begin();}
	inline Scorings::iterator end() {return scorings.end();}
	inline bool contains(const QString& name) {return scorings.contains(name);}
	inline int size() {return scorings.size();}
	
	static inline SeriesScoringDB& getSingleton()
	{
		static SeriesScoringDB instance;
		return instance;
	}
	
private:
	
	SeriesScoringDB();
	
	/// Appends #2, #3, ... in case the desiredName is already used until an unused name is found
	QString getUnusedName(const QString& desiredName);
	
	/// The value can be nullptr if the scoring is not loaded yet
	Scorings scorings;
};

#define seriesScoringDB SeriesScoringDB::getSingleton()

class OrganizerMissingProblem : public Problem
{
Q_OBJECT
public:
	
	OrganizerMissingProblem(SeriesRaceResult* result, const QString& first_name, const QString& last_name, int year, bool isMale);
	
	virtual int getNumSolutions();
	virtual QString getSolutionDescription(int i);
	virtual void applySolution(int i);
	
private:
	
	SeriesRaceResult* result;
	QString first_name;
	QString last_name;
	int year;
	bool isMale;
};

#endif
