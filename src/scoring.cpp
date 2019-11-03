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


#include "scoring.h"

#include <assert.h>
#include <math.h>

#include <QDir>
#include <QFile>
#include <qmath.h>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QMessageBox>

#include "resultList.h"
#include "club.h"
#include "location.h"
#include "runner.h"
#include "config.h"

const QString SCORING_SUFFIX = ".xml";

// ### Scoring ###

Scoring::Scoring(const QString& _fileName) : fileName(_fileName)
{
	// Set defaults
	decimal_places = 2;
	
	limit_runners = false;
	custom_categories = false;
	team_scoring = false;
	
	limit_clubs = false;
	limit_regions = false;
	
	Ruleset* default_ruleset = addRuleset();
	default_ruleset->name = tr("[Standard]");
	
	show_single_results_in_category_listing = false;
}
Scoring::~Scoring()
{
	int size = rulesets.size();
	for (int i = 0; i < size; ++i)
	{
		int size2 = rulesets[i]->handicaps.size();
		for (int k = 0; k < size2; ++k)
			delete rulesets[i]->handicaps[k];
		delete rulesets[i];
	}
	
	size = customCategories.size();
	for (int i = 0; i < size; ++i)
		delete customCategories[i];
}

bool Scoring::saveToFile(const QString path_prefix)
{
	QString path = path_prefix + "my scorings/" + fileName + SCORING_SUFFIX;
	QFile file(path + "_new");
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	
	QXmlStreamWriter stream(&file);
	stream.setAutoFormatting(true);
	stream.writeStartDocument();
	stream.writeStartElement("Scoring");
	stream.writeAttribute("installationKey", installationKey);
	stream.writeAttribute("fileFormatVersion", "1");
	stream.writeAttribute("decimal_places", QString::number(decimal_places));
	
	stream.writeEmptyElement("ProcessingSteps");
	stream.writeAttribute("limit_runners", limit_runners ? "yes" : "no");
	stream.writeAttribute("custom_categories", custom_categories ? "yes" : "no");
	stream.writeAttribute("team_scoring", team_scoring ? "yes" : "no");
	
	if (limit_regions)
	{
		stream.writeStartElement("LimitRegions");
		
		for (std::set< Location* >::iterator it = limitToRegions.begin(); it != limitToRegions.end(); ++it)
		{
			Location* location = *it;
			Location* location_parent = locationDB.getParent(location);
			
			stream.writeEmptyElement("AllowedRegion");
			if (location_parent)
			{
				stream.writeAttribute("country", location_parent->getName());
				stream.writeAttribute("countryID", QString::number(location_parent->getID()));
				stream.writeAttribute("region", location->getName());
				stream.writeAttribute("regionID", QString::number(location->getID()));
			}
			else
			{
				stream.writeAttribute("country", location->getName());
				stream.writeAttribute("countryID", QString::number(location->getID()));
			}
		}
		
		stream.writeEndElement();
	}
	if (limit_clubs)
	{
		stream.writeStartElement("LimitClubs");
		
		for (std::set< Club* >::iterator it = limitToClubs.begin(); it != limitToClubs.end(); ++it)
		{
			Club* club = *it;
			
			stream.writeEmptyElement("AllowedClub");
			stream.writeAttribute("name", club->getName());
			stream.writeAttribute("id", QString::number(club->getID()));
			if (club->getCountry())
			{
				stream.writeAttribute("country", club->getCountry()->getName());
				stream.writeAttribute("countryID", QString::number(club->getCountry()->getID()));
			}
			if (club->getProvince())
			{
				stream.writeAttribute("region", club->getProvince()->getName());
				stream.writeAttribute("regionID", QString::number(club->getProvince()->getID()));
			}
		}
		
		stream.writeEndElement();
	}
	
	int size = rulesets.size();
	for (int i = 0; i < size; ++i)
	{
		Ruleset* ruleset = rulesets[i];
		
		stream.writeStartElement("Ruleset");
		stream.writeAttribute("name", ruleset->name);
		
		if (ruleset->rule_type == FixedInterval)
		{
			stream.writeAttribute("type", "fixed interval");
			
			stream.writeAttribute("interval", ruleset->fixedIntervalSettings.interval.toString());
			stream.writeAttribute("lastRunnerPoints", ruleset->fixedIntervalSettings.lastRunnerPoints.toString());
			stream.writeAttribute("countingRunnersPerClub", QString::number(ruleset->fixedIntervalSettings.countingRunnersPerClub));
			stream.writeAttribute("countTeams", ruleset->fixedIntervalSettings.countTeams ? "yes" : "no");
			stream.writeAttribute("disqualifiedRunnersCount", ruleset->fixedIntervalSettings.disqualifiedRunnersCount ? "yes" : "no");
		}
		else if (ruleset->rule_type == TimeRatio)
		{
			stream.writeAttribute("type", "time ratio");
			
			stream.writeAttribute("formulaNumber", QString::number(ruleset->timeRatioSettings.formulaNumber));
			stream.writeAttribute("formula1Factor", ruleset->timeRatioSettings.formula1Factor.toString());
			stream.writeAttribute("formula2Factor", ruleset->timeRatioSettings.formula2Factor.toString());
			stream.writeAttribute("formula3Factor", ruleset->timeRatioSettings.formula3Factor.toString());
			stream.writeAttribute("formula3Bias", ruleset->timeRatioSettings.formula3Bias.toString());
			stream.writeAttribute("formula3AveragePercentage", ruleset->timeRatioSettings.formula3AveragePercentage.toString());
		}
		else if (ruleset->rule_type == PointTable)
		{
			stream.writeAttribute("type", "point table");
			
			QString pointsString = "";
			for (int i = 0; i < (int)ruleset->pointTableSettings.table.size(); ++i)
			{
				pointsString += ruleset->pointTableSettings.table[i].toString();
				if (i < (int)ruleset->pointTableSettings.table.size() - 1)
					pointsString += ", ";
			}
			stream.writeAttribute("points", pointsString);
		}
		else if (ruleset->rule_type == TimePoints)
		{
			stream.writeAttribute("type", "time points");
			
			QString pointsString = "";
			for (int i = 0; i < (int)ruleset->timePointSettings.table.size(); ++i)
			{
				pointsString += ruleset->timePointSettings.table[i].first.toString();
				pointsString += ", ";
				pointsString += ruleset->timePointSettings.table[i].second.toString();
				
				if (i < (int)ruleset->timePointSettings.table.size() - 1)
					pointsString += ", ";
			}
			stream.writeAttribute("points", pointsString);
		}
		
		stream.writeStartElement("Handicapping");
		stream.writeAttribute("enabled", ruleset->handicapping ? "yes" : "no");

		int size = ruleset->handicaps.size();
		for (int i = 0; i < size; ++i)
		{
			Ruleset::HandicapSetting* handicap = ruleset->handicaps[i];
			stream.writeEmptyElement("Handicap");
			stream.writeAttribute("start", QString::number(handicap->ageStart));
			stream.writeAttribute("end", QString::number(handicap->ageEnd));
			stream.writeAttribute("factor", QString::number(handicap->factor));
			stream.writeAttribute("forMale", handicap->forMale ? "yes" : "no");
		}
		
		stream.writeEndElement();
		stream.writeEndElement();
	}
	
	size = customCategories.size();
	for (int i = 0; i < size; ++i)
	{
		CustomCategory* customCategory = customCategories[i];
		
		stream.writeEmptyElement("CustomCategory");
		stream.writeAttribute("name", customCategory->name);
		stream.writeAttribute("number", QString::number(customCategory->number));
		stream.writeAttribute("ruleset", customCategory->ruleset->name);
		stream.writeAttribute("sourceCategories", customCategory->sourceCategories.join(", "));
	}
	
	stream.writeEmptyElement("TeamScoring");
	if (teamExcludeCategories.size() > 0)
		stream.writeAttribute("excludeCategories", teamExcludeCategories.join(","));
	stream.writeAttribute("show_single_results_in_category_listing", show_single_results_in_category_listing ? "yes" : "no");
	
	stream.writeEndElement();
	stream.writeEndDocument();
	
	QFile::remove(path);
	file.rename(path);
	
	return true;
}
bool Scoring::loadFromFile(QWidget* dialogParent)
{
	QFile file("my scorings/" + fileName + SCORING_SUFFIX);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::warning(dialogParent, tr("Error"), tr("Could not load scoring: %1 because the file could not be opened!").arg(fileName));
		return false;
	}
	
	ProblemWidget* problemWidget = new ProblemWidget();
	
	rulesets.clear();	// Delete standard ruleset
	Ruleset* last_ruleset = nullptr;
	bool use_ids = false;
	
	QXmlStreamReader stream(&file);
	while (!stream.atEnd())
	{
		stream.readNext();
		if (stream.tokenType() != QXmlStreamReader::StartElement)
			continue;
		
		if (stream.name() == "Scoring")
		{
			decimal_places = stream.attributes().value("decimal_places").toString().toInt();
			if (stream.attributes().value("installationKey").toString().compare(installationKey) == 0)
				use_ids = true;
		}
		else if (stream.name() == "ProcessingSteps")
		{
			limit_runners = stream.attributes().value("limit_runners").toString() == "yes";
			custom_categories = stream.attributes().value("custom_categories").toString() == "yes";
			team_scoring = stream.attributes().value("team_scoring").toString() == "yes";
		}
		else if (stream.name() == "LimitRegions")
			limit_regions = true;
		else if (stream.name() == "AllowedRegion")
		{
			Location* country = nullptr;
			Location* province = nullptr;
			
			QStringRef ref;
			if (use_ids)
			{
				ref = stream.attributes().value("countryID");
				assert(!ref.isEmpty());
				country = locationDB.getByID(ref.toString().toInt());
				
				if (country)
				{
					ref = stream.attributes().value("regionID");
					if (!ref.isEmpty())
						province = locationDB.getByID(ref.toString().toInt());
				}
			}
			else
			{
				ref = stream.attributes().value("country");
				assert(!ref.isEmpty());
				country = locationDB.findCountry(ref.toString());
				if (!country)
					country = locationDB.addCountry(ref.toString());
				
				ref = stream.attributes().value("region");
				if (!ref.isEmpty())
				{
					province = country->findChild(ref.toString());
					if (!province)
						province = locationDB.addProvince(country, ref.toString());
				}
			}
			
			if (country)
				limitToRegions.insert(province ? province : country);
		}
		else if (stream.name() == "LimitClubs")
			limit_clubs = true;
		else if (stream.name() == "AllowedClub")
		{
			if (use_ids)
			{
				Club* club = clubDB.getByID(stream.attributes().value("id").toString().toInt());
				if (club)
					limitToClubs.insert(club);
			}
			else
			{
				bool warn_conflict = true;
				Club* club = clubDB.findClub(stream.attributes().value("name").toString());
				if (!club)
				{
					club = new Club(stream.attributes().value("name").toString());
					warn_conflict = false;
				}
				
				Location* country = nullptr;
				Location* province = nullptr;
				
				QStringRef ref = stream.attributes().value("country");
				if (!ref.isEmpty())
				{
					country = locationDB.findCountry(ref.toString());
					if (!country)
						country = locationDB.addCountry(ref.toString());
				}
				
				ref = stream.attributes().value("region");
				if (!ref.isEmpty())
				{
					province = country->findChild(ref.toString());
					if (!province)
						province = locationDB.addProvince(country, ref.toString());
				}
				
				if (warn_conflict && (club->getCountry() != country || club->getProvince() != province))
					problemWidget->addProblem(new ClubDifferentProblem(club, stream.attributes().value("country").toString(), country, stream.attributes().value("region").toString(), province));
				
				limitToClubs.insert(club);
			}
		}
		else if (stream.name() == "Ruleset")
		{
			Ruleset* ruleset = addRuleset();
			last_ruleset = ruleset;
			ruleset->name = stream.attributes().value("name").toString();
			
			QString type = stream.attributes().value("type").toString();
			if (type == "fixed interval")
			{
				ruleset->rule_type = FixedInterval;
				ruleset->fixedIntervalSettings.interval = stream.attributes().value("interval").toString().toDouble();
				ruleset->fixedIntervalSettings.lastRunnerPoints = stream.attributes().value("lastRunnerPoints").toString().toDouble();
				ruleset->fixedIntervalSettings.countingRunnersPerClub = stream.attributes().value("countingRunnersPerClub").toString().toInt();
				ruleset->fixedIntervalSettings.countTeams = stream.attributes().value("countTeams").toString() == "yes";
				ruleset->fixedIntervalSettings.disqualifiedRunnersCount = stream.attributes().value("disqualifiedRunnersCount").toString() == "yes";
			}
			else if (type == "time ratio")
			{
				ruleset->rule_type = TimeRatio;
				ruleset->timeRatioSettings.formulaNumber = stream.attributes().value("formulaNumber").toString().toInt();
				ruleset->timeRatioSettings.formula1Factor = stream.attributes().value("formula1Factor").toString().toDouble();
				ruleset->timeRatioSettings.formula2Factor = stream.attributes().value("formula2Factor").toString().toDouble();
				if (!stream.attributes().value("formula3Factor").isEmpty())
				{
					ruleset->timeRatioSettings.formula3Factor = stream.attributes().value("formula3Factor").toString().toDouble();
					ruleset->timeRatioSettings.formula3Bias = stream.attributes().value("formula3Bias").toString().toDouble();
					ruleset->timeRatioSettings.formula3AveragePercentage = stream.attributes().value("formula3AveragePercentage").toString().toDouble();
				}
				else
				{
					ruleset->timeRatioSettings.formula3Factor = 500;
					ruleset->timeRatioSettings.formula3Bias = 1000;
					ruleset->timeRatioSettings.formula3AveragePercentage = 50;
				}
			}
			else if (type == "point table")
			{
				ruleset->rule_type = PointTable;
				ruleset->pointTableSettings.table.clear();
				QStringList list = stream.attributes().value("points").toString().split(",");
				for (int i = 0; i < list.size(); ++i)
					ruleset->pointTableSettings.table.push_back(FPNumber(list[i].toDouble()));
			}
			else if (type == "time points")
			{
				ruleset->rule_type = TimePoints;
				ruleset->timePointSettings.table.clear();
				QStringList list = stream.attributes().value("points").toString().split(",");
				for (int i = 0; i < list.size() - 1; i += 2)
					ruleset->timePointSettings.table.push_back(std::make_pair(FPNumber(list[i].toDouble()), FPNumber(list[i+1].toDouble())));
			}
		}
		else if (stream.name() == "Handicapping")
		{
			last_ruleset->handicapping = stream.attributes().value("enabled") == "yes";
		}
		else if (stream.name() == "Handicap")
		{
			Ruleset::HandicapSetting* new_handicap = new Ruleset::HandicapSetting();
			new_handicap->ageStart = stream.attributes().value("start").toString().toInt();
			new_handicap->ageEnd = stream.attributes().value("end").toString().toInt();
			new_handicap->factor = stream.attributes().value("factor").toString().toDouble();
			new_handicap->forMale = stream.attributes().value("forMale").toString() == "yes";
			last_ruleset->handicaps.push_back(new_handicap);
		}
		else if (stream.name() == "CustomCategory")
		{
			CustomCategory* customCategory = addCustomCategory();
			
			customCategory->name = stream.attributes().value("name").toString();
			customCategory->number = stream.attributes().value("number").toString().toInt();
			customCategory->sourceCategories = stream.attributes().value("sourceCategories").toString().split(",");
			customCategory->ruleset = findRuleset(stream.attributes().value("ruleset").toString());
		}
		else if (stream.name() == "TeamScoring")
		{
			QStringRef ref = stream.attributes().value("excludeCategories");
			if (!ref.isEmpty())
				teamExcludeCategories = ref.toString().split(",");
			show_single_results_in_category_listing = stream.attributes().value("show_single_results_in_category_listing") == "yes";
		}
	}
	
	if (problemWidget->problemCount() > 0)
		problemWidget->showAsDialog(tr("Problems while loading scoring"), dialogParent);
	else
		delete problemWidget;
	
	return true;
}

void Scoring::mergeClubs(Club* src, Club* dest)
{
	std::set< Club* >::iterator it = limitToClubs.find(src);
	if (it != limitToClubs.end())
	{
		limitToClubs.erase(it);
		limitToClubs.insert(dest);
	}
}

Ruleset* Scoring::addRuleset()
{
	Ruleset* new_ruleset = new Ruleset();
	
	// Set defaults
	new_ruleset->name = tr("New ruleset");
	new_ruleset->rule_type = TimeRatio;
	
	new_ruleset->timeRatioSettings.formulaNumber = 0;
	new_ruleset->timeRatioSettings.formula1Factor = 100;
	new_ruleset->timeRatioSettings.formula2Factor = 100;
	new_ruleset->timeRatioSettings.formula3Factor = 500;
	new_ruleset->timeRatioSettings.formula3Bias = 1000;
	new_ruleset->timeRatioSettings.formula3AveragePercentage = 50;
	
	new_ruleset->fixedIntervalSettings.interval = 1;
	new_ruleset->fixedIntervalSettings.lastRunnerPoints = 1;
	new_ruleset->fixedIntervalSettings.disqualifiedRunnersCount = true;
	new_ruleset->fixedIntervalSettings.countTeams = false;
	new_ruleset->fixedIntervalSettings.countingRunnersPerClub = 99999;
	
	new_ruleset->pointTableSettings.table.push_back(FPNumber(100.0));
	
	new_ruleset->timePointSettings.table.push_back(std::make_pair(FPNumber(100.0), FPNumber(25.0)));

	new_ruleset->handicapping = false;
	
	rulesets.push_back(new_ruleset);
	return new_ruleset;
}
void Scoring::deleteRuleset(Ruleset* ruleset)
{
	int size = rulesets.size();
	for (int i = 0; i < size; ++i)
	{
		if (rulesets[i] == ruleset)
		{
			rulesets.erase(rulesets.begin() + i);
			delete ruleset;
			return;
		}
	}
	
	assert(false);
}
Ruleset* Scoring::findRuleset(const QString& name)
{
	int size = rulesets.size();
	for (int i = 0; i < size; ++i)
	{
		if (rulesets[i]->name == name)
			return rulesets[i];
	}
	return nullptr;
}

CustomCategory* Scoring::addCustomCategory()
{
	CustomCategory* new_category = new CustomCategory();
	
	new_category->ruleset = getStandardRuleset();
	new_category->name = tr("New custom category");
	
	// TODO: Better numbering scheme?!
	new_category->number = 0;
	int size = getNumCustomCategories();
	for (int i = 0; i < size; ++i)
	{
		if (getCustomCategory(i)->number < new_category->number)
			new_category->number = getCustomCategory(i)->number;
	}
	new_category->number = new_category->number - 1;
	
	customCategories.push_back(new_category);
	return new_category;
}
void Scoring::deleteCustomCategory(CustomCategory* customCategory)
{
	int size = customCategories.size();
	for (int i = 0; i < size; ++i)
	{
		if (customCategories[i] == customCategory)
		{
			customCategories.erase(customCategories.begin() + i);
			delete customCategory;
			return;
		}
	}
	
	assert(false);
}

void Scoring::calculateScoring(ResultList* results, int race_year, std::vector< ResultList* >& out, int flags)
{
	ResultList* results_work;
	
	// Limit runners
	if (getLimitRunners() && (limit_clubs || limit_regions) && !(flags & IncludeAllRunners))
		results_work = limitRunners(results, false);
	else
		results_work = new ResultList(*results, true);
	
	// Custom categories
	CustomCategoryToRulesetMap catToRulesetMap;
	if (getCustomCategories())
		applyCustomCategories(results_work, catToRulesetMap);
	
	// Sort merged categories together and calculate new ranks
	results_work->calculateRanks(results_work->getTimeColumn(), results_work->getStatusColumn(), true);
	
	// If all runners must be included, mark those which are not in the scoring: remove their rank
	if (flags & IncludeAllRunners)
	{
		ResultList* old_work = results_work;
		results_work = limitRunners(old_work, true);
		delete old_work;
	}
	
	// Handicap
	calculateHandicappedTimes(results_work, out, catToRulesetMap, race_year);
	
	// Calculate points
	ResultList* singleResults = calculatePoints(results_work, out, catToRulesetMap);
	
	// Calculate team points
	if (getTeamScoring())
		calculateTeamScoring(singleResults, out);
	
	delete results_work;
}

void Scoring::calculateHandicappedTimes(ResultList* results, std::vector< ResultList* >& out, Scoring::CustomCategoryToRulesetMap& catToRulesetMap, int race_year)
{
	// Check if any ruleset uses handicap
	int size = rulesets.size();
	int i;
	for (i = 0; i < size; ++i)
	{
		if (rulesets[i]->handicapping)
			break;
	}
	if (i == size)
		return;
	
	int colHandicappedTime = results->addColumn(ResultList::ColumnTime, tr("Handicapped time"));
	
	bool no_category_yet = true;
	AbstractCategory* current_category = nullptr;
	Ruleset* current_ruleset = nullptr;
	int num_rows = results->rowCount();
	for (int i = 0; i < num_rows; ++i)
	{
		AbstractCategory* category = static_cast<AbstractCategory*>(results->getData(i, results->getCategoryColumn()).value<void*>());
		if (no_category_yet || current_category != category)
		{
			no_category_yet = false;
			current_category = category;
			
			// A new category starts
			CustomCategoryToRulesetMap::iterator it = catToRulesetMap.find(current_category);
			if (it != catToRulesetMap.end())
				current_ruleset = it->second;
			else
				current_ruleset = getStandardRuleset();
		}
		
		double seconds = results->getData(i, results->getTimeColumn()).toInt() * 0.01;
		if (current_ruleset->handicapping)
		{
			Runner* runner = static_cast<Runner*>(results->getData(i, results->getFirstRunnerColumn()).value<void*>());
			int size = current_ruleset->handicaps.size();
			for (int k = 0; k < size; ++k)
			{
				Ruleset::HandicapSetting* handicap = current_ruleset->handicaps[k];
				if (handicap->forMale == runner->isMale() && race_year - runner->getYear() >= handicap->ageStart && race_year - runner->getYear() <= handicap->ageEnd)
				{
					seconds *= handicap->factor;
					break;
				}
			}
		}
		
		results->setData(i, colHandicappedTime, static_cast<int>(seconds * 100 + 0.5));
	}
	
	// Set time column to new column and sort by handicapped times
	results->setTimeColumn(colHandicappedTime);
	results->calculateRanks(results->getTimeColumn(), results->getStatusColumn(), true);
	
	ResultList* handicapped_times = new ResultList(*results, true);
	handicapped_times->setTitle(fileName + " - " + tr("Handicapped times"));
	out.push_back(handicapped_times);
}

void Scoring::applyCustomCategories(ResultList* results, CustomCategoryToRulesetMap& catToRulesetMap)
{
	// Create custom categories map and custom category to ruleset map
	typedef std::map< QString, CustomCategory* > CustomCategoryMap;
	CustomCategoryMap customCategoryMap;
	int size = getNumCustomCategories();
	for (int i = 0; i < size; ++i)
	{
		CustomCategory* customCategory = getCustomCategory(i);
		int sourceSize = customCategory->sourceCategories.size();
		for (int k = 0; k < sourceSize; ++k)
			customCategoryMap.insert(CustomCategoryMap::value_type(customCategory->sourceCategories[k].toLower(), customCategory));
		
		catToRulesetMap.insert(CustomCategoryToRulesetMap::value_type(customCategory, customCategory->ruleset));
	}
	
	// Change categories to custom categories
	bool no_category_yet = true;
	AbstractCategory* current_category = nullptr;
	AbstractCategory* resulting_category = nullptr;	// either the original category or a custom category
	
	int num_rows = results->rowCount();
	for (int i = 0; i < num_rows; ++i)
	{
		AbstractCategory* category = static_cast<AbstractCategory*>(results->getData(i, results->getCategoryColumn()).value<void*>());
		if (no_category_yet || current_category != category)
		{
			no_category_yet = false;
			current_category = category;
			
			// A new category starts
			CustomCategoryMap::iterator it = customCategoryMap.find(current_category->name.toLower());
			if (it == customCategoryMap.end())
				resulting_category = current_category;
			else
				resulting_category = it->second;
		}
		
		if (resulting_category != current_category)
			results->setData(i, results->getCategoryColumn(), qVariantFromValue<void*>(resulting_category));
	}
}
ResultList* Scoring::limitRunners(ResultList* results, bool include_all)
{
	ResultList* results_work = new ResultList(*results, false);
	
	int rank_adjust = 0;
	int num_rows = results->rowCount();
	int num_colums = results->columnCount();
	for (int i = 0; i < num_rows; ++i)
	{
		if (results->getData(i, results->getRankColumn()).toInt() == 1)
			rank_adjust = i;
		
		if (!include_all && !results->getData(i, results->getRankColumn()).isValid())
			continue;	// skip noncompetitive entries if !include_all
		
		bool include = true;
		if (limit_clubs || limit_regions)
		{
			Club* club = static_cast<Club*>(results->getData(i, results->getClubColumn()).value<void*>());
			if (limit_clubs && !isAllowedClub(club))
			{
				if (include_all)
					include = false;
				else
					continue;
			}
			
			if (include && limit_regions)
			{
				Location* location = club->getProvince();
				if (location == nullptr)
					location = club->getCountry();
				
				if (!isAllowedRegion(location))
				{
					if (include_all)
						include = false;
					else
						continue;
				}
			}
		}
		
		AbstractCategory* category = static_cast<AbstractCategory*>(results->getData(i, results->getCategoryColumn()).value<void*>());
		if (!include_all && teamExcludeCategories.contains(category->name, Qt::CaseInsensitive))
			continue;

		// Row is ok, copy it to results_work
		int pos = results_work->addRow();
		for (int k = 0; k < num_colums; ++k)
			results_work->setData(pos, k, results->getData(i, k));
		
		// Adjust rank
		bool valid_rank = results_work->getData(pos, results_work->getRankColumn()).isValid() && results_work->getData(pos, results_work->getRankColumn()).toInt() > 0;
		if (include)
		{
			if (valid_rank)
				results_work->setData(pos, results_work->getRankColumn(), results_work->getData(pos, results_work->getRankColumn()).toInt() - i + rank_adjust);
			++rank_adjust;
		}
		else
		{
			if (valid_rank)
				results_work->setData(pos, results_work->getRankColumn(), QVariant());
			else
				results_work->setData(pos, results_work->getRankColumn(), -2);
		}
	}
	
	// This is necessary because the case that one ranked and one unranked runner had the same rank before the limit runner operation is not handled above
	results_work->calculateRanks(results_work->getTimeColumn(), results_work->getStatusColumn(), true, true);	// TODO: This makes the "rank adjust" stuff above unnecessary
	
	return results_work;
}

ResultList* Scoring::calculatePoints(ResultList* results, std::vector< ResultList* >& out, Scoring::CustomCategoryToRulesetMap& catToRulesetMap)
{
	ResultList* singleResults = new ResultList(fileName + " - " + tr("Single results"), decimal_places);
	int colCategory = singleResults->addColumn(ResultList::ColumnCategory);
	int colRank = singleResults->addColumn(ResultList::ColumnRank);
	int colRunner = singleResults->addColumn(ResultList::ColumnRunner);
	if (results->getFirstRunnerColumn() != results->getLastRunnerColumn())
	{
		singleResults->addColumn(ResultList::ColumnTime, tr("Runner time"));
		for (int i = results->getFirstRunnerColumn() + 2; i <= results->getLastRunnerColumn(); i += 2)
		{
			singleResults->addColumn(ResultList::ColumnRunner);
			singleResults->addColumn(ResultList::ColumnTime, tr("Runner time"));
		}
	}
	int num_runners = ((results->getLastRunnerColumn() - results->getFirstRunnerColumn()) / 2) + 1;
	int colClub = singleResults->addColumn(ResultList::ColumnClub);
	int colTime = singleResults->addColumn(ResultList::ColumnTime);
	singleResults->setTimeColumn(colTime);
	// TODO: Original times column, if handicapping enabled; maybe also the handicapping factor
	int colPoints = singleResults->addColumn(ResultList::ColumnPoints);
	
	int roundFactor = static_cast<int>(pow(10.0, decimal_places) + 0.5);
	
	double winner_seconds = -1;
	double averaged_seconds = -1;
	double current_points = 0;
	double previous_points = -1;
	int previous_point_rank = -1;
	bool no_category_yet = true;
	AbstractCategory* current_category = nullptr;
	Ruleset* current_ruleset = nullptr;
	std::map< Club*, int > runnersPerClub;
	bool excluded_category = false;
	
	int num_rows = results->rowCount();
	for (int i = 0; i < num_rows; ++i)
	{
		// ResultDidNotStart entries should not show up in the points table; TODO: make configurable?
		ResultList::ResultType resultType = static_cast<ResultList::ResultType>(results->getData(i, results->getStatusColumn()).toInt());
		if (resultType == ResultList::ResultDidNotStart)
			continue;
		
		AbstractCategory* category = static_cast<AbstractCategory*>(results->getData(i, results->getCategoryColumn()).value<void*>());
		if (no_category_yet || current_category != category)
		{
			no_category_yet = false;
			current_category = category;
			
			// A new category starts
			excluded_category = teamExcludeCategories.contains(current_category->name, Qt::CaseInsensitive);
			
			CustomCategoryToRulesetMap::iterator it = catToRulesetMap.find(current_category);
			if (it != catToRulesetMap.end())
				current_ruleset = it->second;
			else
				current_ruleset = getStandardRuleset();
			
			if (current_ruleset->rule_type == FixedInterval)
			{
				// Determine number of counting runners and thus points for first runner
				current_points = current_ruleset->fixedIntervalSettings.lastRunnerPoints.toDouble() - current_ruleset->fixedIntervalSettings.interval.toDouble();
				runnersPerClub.clear();
				for (int k = i; k < num_rows; ++k)
				{
					AbstractCategory* k_category = static_cast<AbstractCategory*>(results->getData(k, results->getCategoryColumn()).value<void*>());
					if (k_category != current_category)
						break;
					
					if (!results->getData(k, results->getRankColumn()).isValid() || results->getData(k, results->getRankColumn()).toInt() == -2)
						continue;	// runner not in scoring
					
					ResultList::ResultType k_resultType = static_cast<ResultList::ResultType>(results->getData(k, results->getStatusColumn()).toInt());
					if (k_resultType == ResultList::ResultDidNotStart)
						continue;
					if (k_resultType != ResultList::ResultOk && !current_ruleset->fixedIntervalSettings.disqualifiedRunnersCount)
						continue;
					
					Club* club = static_cast<Club*>(results->getData(k, results->getClubColumn()).value<void*>());
					std::map< Club*, int >::iterator rit = runnersPerClub.find(club);
					if (rit == runnersPerClub.end())
						runnersPerClub.insert(std::map< Club*, int >::value_type(club, 1));
					else
					{
						if (rit->second >= current_ruleset->fixedIntervalSettings.countingRunnersPerClub)
							continue;
						
						++rit->second;
					}
					if (!current_ruleset->fixedIntervalSettings.countTeams)
						current_points += current_ruleset->fixedIntervalSettings.interval.toDouble();
				}
				if (current_ruleset->fixedIntervalSettings.countTeams)
					current_points += runnersPerClub.size() * current_ruleset->fixedIntervalSettings.countingRunnersPerClub * current_ruleset->fixedIntervalSettings.interval.toDouble();
				runnersPerClub.clear();
			}
			else if (current_ruleset->rule_type == TimeRatio && current_ruleset->timeRatioSettings.formulaNumber == 2)
			{
				// Calculate averaged_seconds
				int num_started_runners = 0;
				std::vector<double> averaged_times;
				
				for (int k = i; k < num_rows; ++k)
				{
					AbstractCategory* k_category = static_cast<AbstractCategory*>(results->getData(k, results->getCategoryColumn()).value<void*>());
					if (k_category != current_category)
						break;
					
					if (!results->getData(k, results->getRankColumn()).isValid() || results->getData(k, results->getRankColumn()).toInt() == -2)
						continue;	// runner not in scoring
					
					ResultList::ResultType k_resultType = static_cast<ResultList::ResultType>(results->getData(k, results->getStatusColumn()).toInt());
					if (k_resultType != ResultList::ResultDidNotStart)
						++num_started_runners;
					if (k_resultType != ResultList::ResultOk)
						continue;
					
					averaged_times.push_back(results->getData(k, results->getTimeColumn()).toInt() * 0.01);
				}
				
				int num_averaged_times = qCeil(current_ruleset->timeRatioSettings.formula3AveragePercentage.toDouble() * 0.01 * num_started_runners);
				if (num_averaged_times < (int)averaged_times.size())
					averaged_times.resize(num_averaged_times);
				
				averaged_seconds = 0;
				for (size_t k = 0; k < averaged_times.size(); ++k)
					averaged_seconds += averaged_times[k];
				averaged_seconds = averaged_seconds / averaged_times.size();
			}
		}
		
		// Start creating new row
		int new_row = singleResults->addRow();
		singleResults->setData(new_row, colCategory, results->getData(i, results->getCategoryColumn()));
		singleResults->setData(new_row, colRank, results->getData(i, results->getRankColumn()));
		for (int k = 0; k < num_runners; ++k)
		{
			singleResults->setData(new_row, colRunner + 2*k, results->getData(i, results->getFirstRunnerColumn() + 2*k));
			if (num_runners > 1)
				singleResults->setData(new_row, colRunner + 2*k + 1, results->getData(i, results->getFirstRunnerColumn() + 2*k + 1));
		}
		singleResults->setData(new_row, colClub, results->getData(i, results->getClubColumn()));
		singleResults->setData(new_row, colTime, results->getData(i, results->getTimeColumn()));
		
		double seconds = results->getData(i, results->getTimeColumn()).toInt() * 0.01;
		double points = -1;
		
		if (!excluded_category && results->getData(i, results->getRankColumn()).isValid() && results->getData(i, results->getRankColumn()).toInt() != -2)
		{
			// If this is the winner, get winner time
			int time_based_rank = results->getData(i, results->getRankColumn()).toInt();
			if (time_based_rank == 1)
				winner_seconds = seconds;
			
			// Calculate and set points
			if (resultType != ResultList::ResultOk)
				points = -1;
			else if (current_ruleset->rule_type == TimeRatio)
			{
				if (current_ruleset->timeRatioSettings.formulaNumber == 0)
					points = current_ruleset->timeRatioSettings.formula1Factor.toDouble() * (winner_seconds / seconds);
				else if (current_ruleset->timeRatioSettings.formulaNumber == 1)
					points = current_ruleset->timeRatioSettings.formula2Factor.toDouble() * qMax(0.0, (2 - seconds / winner_seconds));
				else if (current_ruleset->timeRatioSettings.formulaNumber == 2)
				{
					points = current_ruleset->timeRatioSettings.formula3Bias.toDouble() + current_ruleset->timeRatioSettings.formula3Factor.toDouble() *
						(averaged_seconds - seconds) / averaged_seconds;
					points = qMax(0.0, points);
				}
			}
			else if (current_ruleset->rule_type == FixedInterval)
			{
				bool counting_runner = true;
				
				Club* club = static_cast<Club*>(results->getData(i, results->getClubColumn()).value<void*>());
				std::map< Club*, int >::iterator rit = runnersPerClub.find(club);
				if (rit == runnersPerClub.end())
					runnersPerClub.insert(std::map< Club*, int >::value_type(club, 1));
				else
				{
					if (rit->second >= current_ruleset->fixedIntervalSettings.countingRunnersPerClub)
						counting_runner = false;
					
					++rit->second;
				}
				
				if (counting_runner)
				{
					if (time_based_rank == previous_point_rank)
						points = previous_points;
					else
						points = current_points;

					current_points -= current_ruleset->fixedIntervalSettings.interval.toDouble();
					previous_points = points;
					previous_point_rank = time_based_rank;
				}
				else
					points = 0;
			}
			else if (current_ruleset->rule_type == PointTable)
			{
				int num_items = current_ruleset->pointTableSettings.table.size();
				points = current_ruleset->pointTableSettings.table[qMin(time_based_rank - 1, num_items - 1)].toDouble();
			}
			else if (current_ruleset->rule_type == TimePoints)
			{
				double percentage_of_winner_time = 100 * (seconds / winner_seconds);
				int num_items = current_ruleset->timePointSettings.table.size();
				int item = 0;
				for (; item < num_items - 1; ++item)
				{
					if (percentage_of_winner_time <= current_ruleset->timePointSettings.table[item].first.toDouble())
						break;
				}
				points = current_ruleset->timePointSettings.table[item].second.toDouble();
			}
			else
				assert(false);
		}
		
		if (points >= 0)
			singleResults->setData(new_row, colPoints, static_cast<int>(points * roundFactor + 0.5));
		else
			singleResults->setData(new_row, colPoints, QVariant());
	}
	
	//singleResults->calculateRanks(colPoints, -1, false);
	
	out.push_back(singleResults);
	return singleResults;
}

ResultList* Scoring::calculateTeamScoring(ResultList* points, std::vector< ResultList* >& out)
{
	int roundFactor = static_cast<int>(pow(10.0, decimal_places) + 0.5);
	
	std::map< Club*, double > pointsPerClub;				// for total result
	std::map< AbstractCategory*, int > categoryToColumn;	// for category results
	std::map< int, AbstractCategory* > columnToCategory;	// for category results
	std::map< Club*, int > clubToRow;						// for category results
	
	// Count categories and clubs
	int num_rows = points->rowCount();
	for (int i = 0; i < num_rows; ++i)
	{
		if (!points->getData(i, points->getRankColumn()).isValid() || points->getData(i, points->getRankColumn()).toInt() == -2)
			continue;
		//if (!points->getData(i, points->getPointsColumn()).isValid())
		//	continue;
		
		AbstractCategory* cat = static_cast<AbstractCategory*>(points->getData(i, points->getCategoryColumn()).value<void*>());
		assert(cat != nullptr);
		if (teamExcludeCategories.contains(cat->name, Qt::CaseInsensitive))
			continue;	// NOTE: could speed this up if it would be *really* neccessary, also see below ...
		if (categoryToColumn.find(cat) == categoryToColumn.end())
		{
			int column = categoryToColumn.size();
			categoryToColumn.insert(std::make_pair(cat, column));
			columnToCategory.insert(std::make_pair(column, cat));
		}
		
		Club* club = static_cast<Club*>(points->getData(i, points->getClubColumn()).value<void*>());
		if (clubToRow.find(club) == clubToRow.end())
			clubToRow.insert(std::make_pair(club, clubToRow.size()));
	}
	
	if (categoryToColumn.size() == 0)
		return nullptr;
	
	std::vector<double>* categoryResultLists = nullptr;
	if (show_single_results_in_category_listing)
		categoryResultLists = new std::vector<double>[clubToRow.size() * categoryToColumn.size()];
	
	double** categoryResults = new double*[clubToRow.size()];
	for (int i = 0; i < (int)clubToRow.size(); ++i)
	{
		categoryResults[i] = new double[categoryToColumn.size()];
		for (int k = 0; k < (int)categoryToColumn.size(); ++k)
			categoryResults[i][k] = -1;	// negative means: no runner in this category
	}
	
	// Sum up results
	for (int i = 0; i < num_rows; ++i)
	{
		if (!points->getData(i, points->getRankColumn()).isValid() || points->getData(i, points->getRankColumn()).toInt() == -2)
			continue;
		//if (!points->getData(i, points->getPointsColumn()).isValid())
		//	continue;
		
		AbstractCategory* cat = static_cast<AbstractCategory*>(points->getData(i, points->getCategoryColumn()).value<void*>());
		if (teamExcludeCategories.contains(cat->name, Qt::CaseInsensitive))
			continue;	// NOTE: could speed this up if it would be *really* neccessary, also see above ...
		
		Club* club = static_cast<Club*>(points->getData(i, points->getClubColumn()).value<void*>());
		QVariant p_variant = points->getData(i, points->getPointsColumn());
		double p;
		if (p_variant.isValid())
			p = p_variant.toInt() / (double)roundFactor;
		else
			p = 0;
		
		std::map< Club*, double >::iterator it = pointsPerClub.find(club);
		if (it == pointsPerClub.end())
			pointsPerClub.insert(std::map< Club*, double >::value_type(club, p));
		else
			it->second += p;
		
		int row = clubToRow.find(club)->second;
		int column = categoryToColumn.find(cat)->second;
		if (categoryResults[row][column] < 0)
			categoryResults[row][column] = p;
		else
			categoryResults[row][column] += p;
		
		if (categoryResultLists)
			categoryResultLists[row * categoryToColumn.size() + column].push_back(p);
	}
	
	// Find maximum number of results in every column
	int* maxResultsPerColumn = nullptr;
	if (categoryResultLists)
	{
		int num_rows = (int)clubToRow.size();
		maxResultsPerColumn = new int[categoryToColumn.size()];
		for (int column = 0; column < (int)categoryToColumn.size(); ++column)
		{
			bool zero_found;
			do
			{
				maxResultsPerColumn[column] = 0;
				for (int row = 0; row < num_rows; ++row)
				{
					int count = (int)categoryResultLists[row * categoryToColumn.size() + column].size();
					if (count > maxResultsPerColumn[column])
						maxResultsPerColumn[column] = count;
				}
				assert(maxResultsPerColumn[column] < 1000);
				
				// Do all max results rows contain a "0"? If yes, delete it and try again ...
				zero_found = false;
				for (int row = 0; row < num_rows; ++row)
				{
					std::vector<double>* list = &categoryResultLists[row * categoryToColumn.size() + column];
					int count = (int)list->size();
					if (count == maxResultsPerColumn[column])
					{
						// Check for a zero
						zero_found = false;
						for (int i = 0; i < count; ++i)
						{
							if (list->at(i) == 0)
							{
								zero_found = true;
								break;
							}
						}
						if (!zero_found)
							break;
					}
				}
				if (zero_found)
				{
					// Delete one zero from every row with max entries and try again ...
					for (int row = 0; row < num_rows; ++row)
					{
						std::vector<double>* list = &categoryResultLists[row * categoryToColumn.size() + column];
						int count = (int)list->size();
						if (count == maxResultsPerColumn[column])
						{
							// Check for a zero
							for (int i = 0; i < count; ++i)
							{
								if (list->at(i) == 0)
								{
									list->erase(list->begin() + i);
									break;
								}
							}
						}
					}
				}
			} while (zero_found);
		}
	}
	
	// Sort results by summed points
	std::multimap< double, Club* > pointToClubMap;
	for (std::map< Club*, double >::iterator it = pointsPerClub.begin(); it != pointsPerClub.end(); ++it)
		pointToClubMap.insert(std::multimap< double, Club* >::value_type(it->second, it->first));
	
	// Create result tables
	ResultList* teamResults = new ResultList(fileName + " - " + tr("Team results"), decimal_places);
	int colRank = teamResults->addColumn(ResultList::ColumnRank);
	int colClub = teamResults->addColumn(ResultList::ColumnClub);
	int colPoints = teamResults->addColumn(ResultList::ColumnPoints);
	
	ResultList* teamResultsByCat = new ResultList(fileName + " - " + tr("Team results by category"), decimal_places);
	int colCatRank = teamResultsByCat->addColumn(ResultList::ColumnRank);
	int colCatClub = teamResultsByCat->addColumn(ResultList::ColumnClub);
	int colCatCategoryStart = colCatClub + 1;
	if (show_single_results_in_category_listing)
	{
		for (int i = 0; i < (int)categoryToColumn.size(); ++i)
		{
			AbstractCategory* cat = columnToCategory[i];
			if (maxResultsPerColumn[i] == 1)
				teamResultsByCat->addColumn(ResultList::ColumnPoints, cat->name);
			else
				for (int k = 0; k < maxResultsPerColumn[i]; ++k)
					teamResultsByCat->addColumn(ResultList::ColumnPoints, cat->name + " (" + QString::number(k+1) + ")");
		}
	}
	else
	{
		for (int i = 0; i < (int)categoryToColumn.size(); ++i)
			teamResultsByCat->addColumn(ResultList::ColumnPoints);
		for (std::map< AbstractCategory*, int >::iterator it = categoryToColumn.begin(); it != categoryToColumn.end(); ++it)
			teamResultsByCat->setColumnLabel(colCatCategoryStart + it->second, it->first->name);
	}
	int colCatPoints = teamResultsByCat->addColumn(ResultList::ColumnPoints);
	teamResultsByCat->setPointsColumn(colCatPoints);
	teamResultsByCat->setAdditionalStartColumn(colCatCategoryStart);
	teamResultsByCat->setAdditionalEndColumn(colCatPoints - 1);
	
	int prev_points = -1;
	for (std::multimap< double, Club* >::reverse_iterator it = pointToClubMap.rbegin(); it != pointToClubMap.rend(); ++it)
	{
		int cur_points = static_cast<int>(it->first * roundFactor + 0.5);
		int i = teamResults->addRow();
		teamResultsByCat->addRow();
		
		int rank;
		if (cur_points == prev_points)
			rank = teamResults->getData(i - 1, colRank).toInt();
		else
			rank = i + 1;
		teamResults->setData(i, colRank, rank);
		teamResultsByCat->setData(i, colCatRank, rank);
		teamResults->setData(i, colClub, qVariantFromValue<void*>(it->second));
		teamResultsByCat->setData(i, colCatClub, qVariantFromValue<void*>(it->second));
		teamResults->setData(i, colPoints, cur_points);
		teamResultsByCat->setData(i, colCatPoints, cur_points);
		
		int src_row = clubToRow.find(it->second)->second;
		int offset = 0;
		for (int c = 0; c < (int)categoryToColumn.size(); ++c)
		{
			if (show_single_results_in_category_listing)
			{
				int count = (int)categoryResultLists[src_row * categoryToColumn.size() + c].size();
				for (int k = 0; k < count; ++k)
					teamResultsByCat->setData(i, colCatCategoryStart + c + offset + k, static_cast<int>(categoryResultLists[src_row * categoryToColumn.size() + c][k] * roundFactor + 0.5));
				offset += maxResultsPerColumn[c] - 1;
			}
			else
			{
				if (categoryResults[src_row][c] >= 0)
					teamResultsByCat->setData(i, colCatCategoryStart + c, static_cast<int>(categoryResults[src_row][c] * roundFactor + 0.5));
			}
		}
		
		prev_points = cur_points;
	}
	
	out.push_back(teamResults);
	out.push_back(teamResultsByCat);
	
	for (int i = 0; i < (int)clubToRow.size(); ++i)
		delete[] categoryResults[i];
	delete[] categoryResults;
	
	delete[] categoryResultLists;
	delete[] maxResultsPerColumn;
	
	return teamResults;
}

// ### ScoringDB ###

ScoringDB::ScoringDB()
{
	const QString dir_name = "my scorings";
	
	QDir dir(dir_name);
	if (!dir.exists())
	{
		QDir curDir;
		curDir.mkdir(dir_name);
		dir = QDir(dir_name);
	}
	QStringList nameFilters;
	nameFilters << ("*" + SCORING_SUFFIX);
	QStringList myScorings = dir.entryList(nameFilters, QDir::Files | QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::Name);
	
	for (int i = 0; i < myScorings.size(); ++i)
		scorings.insert(myScorings.at(i).left(myScorings.at(i).size() - SCORING_SUFFIX.size()), nullptr);
	
	// TODO: dir watcher
}

Scoring* ScoringDB::getOrLoadScoring(const QString& name, QWidget* dialogParent)
{
	Scorings::iterator it = scorings.find(name);
	if (it == scorings.end())
		return nullptr;
	
	if (*it)
		return *it;

	// Scoring not loaded yet
	Scoring* loadedScoring = new Scoring(name);
	if (!loadedScoring->loadFromFile(dialogParent))
	{
		delete loadedScoring;
		return nullptr;
	}
	scorings.insert(name, loadedScoring);
	
	return loadedScoring;
}

bool ScoringDB::saveToFile(const QString path_prefix)
{
	for (Scorings::iterator it = scorings.begin(); it != scorings.end(); ++it)
	{
		if (!*it)
			continue;
		
		(*it)->saveToFile(path_prefix);
	}
	return true;
}

void ScoringDB::mergeClubs(Club* src, Club* dest)
{
	for (Scorings::iterator it = scorings.begin(); it != scorings.end(); ++it)
	{
		if (!*it)
			continue;
		
		(*it)->mergeClubs(src, dest);
	}
}

Scoring* ScoringDB::createNewScoring(const QString& desiredName)
{
	QString name = getUnusedName(desiredName);
	Scoring* new_scoring = new Scoring(name);
	scorings.insert(name, new_scoring);
	return new_scoring;
}

void ScoringDB::changeScoringName(const QString& old_name, const QString& new_name)
{
	assert(old_name != new_name);
	Scorings::iterator it = scorings.find(old_name);
	if (it == scorings.end())
		return;
	
	scorings.insert(new_name, it.value());
	scorings.erase(it);
	QFile::rename("my scorings/" + old_name + ".xml", "my scorings/" + new_name + ".xml");
}

void ScoringDB::deleteScoring(const QString& name)
{
	scorings.remove(name);
	QFile::remove("my scorings/" + name + SCORING_SUFFIX);
}

QString ScoringDB::getUnusedName(const QString& desiredName)
{
	if (!contains(desiredName))
		return desiredName;
	
	for (int i = 2; i < 9999; ++i)
	{
		QString name = desiredName + " #" + QString::number(i);
		if (!contains(name))
			return name;
	}
	
	// Should practically never happen
	return desiredName;
}

// ### ClubDifferentProblem ###

ClubDifferentProblem::ClubDifferentProblem(Club* club, const QString& country_text, Location* country, const QString& province_text, Location* province) : club(club), country(country), province(province)
{
	database_location = QString("%1, %2").arg(club->getCountry() ? club->getCountry()->getName() : tr("not specified"),
	                                          club->getProvince() ? club->getProvince()->getName() : tr("not specified"));
	scoring_location = QString("%1, %2").arg(country ? country->getName() : (country_text.isEmpty() ? tr("not specified") : tr("not found")),
	                                         province ? province->getName() : (province_text.isEmpty() ? tr("not specified") : tr("not found")));
	description = tr("Club %1 is located at %2 according to the database, but at %3 according to the scoring")
					.arg(club->getName(), database_location, scoring_location);
}
int ClubDifferentProblem::getNumSolutions()
{
	return 2;
}
QString ClubDifferentProblem::getSolutionDescription(int i)
{
	if (i == 0) return tr("Keep %1").arg(database_location);
	else /*if (i == 1)*/ return tr("Use %1").arg(scoring_location);
}
void ClubDifferentProblem::applySolution(int i)
{
	if (i == 1)
		club->setLocation(country, province);
}
