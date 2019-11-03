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


#ifndef SCORING_H
#define SCORING_H

#include <vector>
#include <set>

#include <QMap>
#include <QString>
#include <QObject>
#include <QStringList>

#include "util.h"
#include "event.h"
#include "problemWidget.h"

class Club;
class Location;
class ResultList;

enum RuleType
{
	// NOTE: The numbers are used as indexes in the QStackedLayout of the scoring dialog GUI and
	// as radio button indexes for these options, don't change them without adjusting this!
	TimeRatio = 0,
	FixedInterval = 1,
	PointTable = 2,
	TimePoints = 3
};

struct Ruleset : public QObject
{
Q_OBJECT
public:

	struct TimeRatioSettings
	{
		int formulaNumber;
		
		FPNumber formula1Factor;
		FPNumber formula2Factor;
		FPNumber formula3Factor;
		FPNumber formula3Bias;
		FPNumber formula3AveragePercentage;
	};
	struct FixedIntervalSettings
	{
		FPNumber interval;
		FPNumber lastRunnerPoints;
		int countingRunnersPerClub;
		bool countTeams;
		bool disqualifiedRunnersCount;
	};
	struct PointTableSettings
	{
		std::vector<FPNumber> table;
	};
	struct TimePointSettings
	{
		std::vector< std::pair<FPNumber, FPNumber> > table;
	};
	struct HandicapSetting
	{
		bool forMale;
		int ageStart;
		int ageEnd;
		double factor;
		
		inline QString getDesc() const {return (forMale ? tr("M") : tr("W")) + QString::number(ageStart) + "-" + QString::number(ageEnd);}
	};
	
	QString name;
	RuleType rule_type;
	
	TimeRatioSettings timeRatioSettings;
	FixedIntervalSettings fixedIntervalSettings;
	PointTableSettings pointTableSettings;
	TimePointSettings timePointSettings;
	
	bool handicapping;
	std::vector<HandicapSetting*> handicaps;
};

class CustomCategory : public AbstractCategory
{
public:
	
	QStringList sourceCategories;
	Ruleset* ruleset;
};

/// Scorings take race results as input and calculate handicapped times or points for every runner (and possibly team)
class Scoring : public QObject
{
Q_OBJECT
public:
	
	enum CalculationFlags
	{
		IncludeAllRunners = 1
	};
	
	/// Creates a new scoring
	Scoring(const QString& _fileName);
	~Scoring();
	
	/// Calculates the scoring, stores the resulting ResultLists in out; flags from Scoring::CalculationFlags
	void calculateScoring(ResultList* results, int race_year, std::vector< ResultList* >& out, int flags = 0);
	
	bool loadFromFile(QWidget* dialogParent);
	bool saveToFile(const QString path_prefix = "");
	
	/// Replace all ocurrences of club src with dest
	void mergeClubs(Club* src, Club* dest);
	
	// Setters / Getters
	inline void setDecimalPlaces(int value) {decimal_places = value;}
	inline int getDecimalPlaces() const {return decimal_places;}
	
	inline void setLimitRunners(bool enable) {limit_runners = enable;}
	inline bool getLimitRunners() const {return limit_runners;}
	inline void setCustomCategories(bool enable) {custom_categories = enable;}
	inline bool getCustomCategories() const {return custom_categories;}
	inline void setTeamScoring(bool enable) {team_scoring = enable;}
	inline bool getTeamScoring() const {return team_scoring;}
	
	inline void setLimitRegions(bool value) {limit_regions = value;}
	inline bool getLimitRegions() const {return limit_regions;}
	inline void setAllowedRegion(Location* l, bool allow) {if (allow) limitToRegions.insert(l); else limitToRegions.erase(l);}
	inline bool isAllowedRegion(Location* l) const {return limitToRegions.find(l) != limitToRegions.end();}
	inline void setLimitClubs(bool value) {limit_clubs = value;}
	inline bool getLimitClubs() const {return limit_clubs;}
	inline void setAllowedClub(Club* c, bool allow) {if (allow) limitToClubs.insert(c); else limitToClubs.erase(c);}
	inline bool isAllowedClub(Club* c) const {return limitToClubs.find(c) != limitToClubs.end();}
	
	Ruleset* addRuleset();
	inline int getNumRulesets() const {return rulesets.size();}
	inline Ruleset* getRuleset(int i) {return rulesets[i];}
	inline Ruleset* getStandardRuleset() {return rulesets[0];}
	Ruleset* findRuleset(const QString& name);
	void deleteRuleset(Ruleset* ruleset);
	
	CustomCategory* addCustomCategory();
	inline int getNumCustomCategories() const {return customCategories.size();}
	inline CustomCategory* getCustomCategory(int i) {return customCategories[i];}
	void deleteCustomCategory(CustomCategory* customCategory);
	
	inline QStringList& getTeamExcludeCategories() {return teamExcludeCategories;}
	inline void setShowSingleResultsInCategoryListing(bool enable) {show_single_results_in_category_listing = enable;}
	inline bool getShowSingleResultsInCategoryListing() const {return show_single_results_in_category_listing;}
	
	inline void setFileName(const QString& value) {fileName = value;}
	inline const QString& getFileName() const {return fileName;}
	
private:
	
	typedef std::map< AbstractCategory*, Ruleset* > CustomCategoryToRulesetMap;
	
	ResultList* limitRunners(ResultList* results, bool include_all);
	void applyCustomCategories(ResultList* results, CustomCategoryToRulesetMap& catToRulesetMap);
	ResultList* calculatePoints(ResultList* results, std::vector< ResultList* >& out, Scoring::CustomCategoryToRulesetMap& catToRulesetMap);
	ResultList* calculateTeamScoring(ResultList* points, std::vector< ResultList* >& out);
	void calculateHandicappedTimes(ResultList* results, std::vector< ResultList* >& out, Scoring::CustomCategoryToRulesetMap& catToRulesetMap, int race_year);
	
	// Number of digits after the comma
	int decimal_places;
	
	// Enabled processing steps
	bool limit_runners;
	bool custom_categories;
	bool team_scoring;
	
	// Limit runner settings
	bool limit_regions;
	std::set< Location* > limitToRegions;
	bool limit_clubs;
	std::set< Club* > limitToClubs;
	
	// Point calculation rules
	typedef std::vector< Ruleset* > RulesetList;
	RulesetList rulesets;
	
	// Custom categories
	typedef std::vector< CustomCategory* > CustomCategoryList;
	CustomCategoryList customCategories;
	
	// Team scoring
	QStringList teamExcludeCategories;
	bool show_single_results_in_category_listing;
	
	QString fileName;
};

/// Manages all scorings which were ever used, loads only those that are necessary
class ScoringDB
{
public:
	
	typedef QMap<QString, Scoring*> Scorings;
	
	bool saveToFile(const QString path_prefix = "");
	
	/// Returns the scoring with the given name; loads it if it is not loaded yet.
	/// Returns nullptr if there was an error loading the scoring.
	Scoring* getOrLoadScoring(const QString& name, QWidget* dialogParent);
	
	/// Appends #2, #3, ... in case the desiredName is already used until an unused name is found
	Scoring* createNewScoring(const QString& desiredName);
	
	/// Does nothing if the old name does not exist
	void changeScoringName(const QString& old_name, const QString& new_name);
	
	/// Removes the scoring from the database and deletes the scoring file
	void deleteScoring(const QString& name);
	
	/// Replace all ocurrences of club src with dest
	void mergeClubs(Club* src, Club* dest);
	
	inline Scorings::iterator begin() {return scorings.begin();}
	inline Scorings::iterator end() {return scorings.end();}
	inline bool contains(const QString& name) {return scorings.contains(name);}
	inline int size() {return scorings.size();}
	
	static inline ScoringDB& getSingleton()
	{
		static ScoringDB instance;
		return instance;
	}
	
private:
	
	ScoringDB();
	
	/// Appends #2, #3, ... in case the desiredName is already used until an unused name is found
	QString getUnusedName(const QString& desiredName);
	
	/// The value can be nullptr if the scoring is not loaded yet
	Scorings scorings;
};

#define scoringDB ScoringDB::getSingleton()

class ClubDifferentProblem : public Problem
{
Q_OBJECT
public:
	
	ClubDifferentProblem(Club* club, const QString& country_text, Location* country, const QString& province_text, Location* province);
	
    virtual int getNumSolutions();
    virtual QString getSolutionDescription(int i);
    virtual void applySolution(int i);
	
private:
	
	Club* club;
	Location* country;
	Location* province;
	QString database_location;
	QString scoring_location;
};

#endif
