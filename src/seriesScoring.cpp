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


#include "seriesScoring.h"

#include <assert.h>

#include <QDir>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include "resultList.h"
#include "club.h"
#include "runner.h"
#include "event.h"
#include "config.h"

// ### SeriesScoring ###

SeriesScoring::SeriesScoring(const QString& _fileName) : fileName(_fileName)
{
	result = nullptr;			// no result calculated yet
	result_dirty = false;	// ... but no races have been added either
	
	// set defaults
	count_only_best_runs = false;
	num_counting_runs = 4;
	
	organizer_bonus_type = NoOrganizerBonus;
	organizer_bonus_fixed = 100;
	organizer_bonus_percentage = 90;
	organizer_bonus_counting_runs = 3;
}
SeriesScoring::~SeriesScoring()
{
	for (ResultMap::iterator it = resultMap.begin(); it != resultMap.end(); ++it)
		delete it->second;
	
	delete result;
}

bool SeriesScoring::saveToFile(const QString path_prefix)
{
	QString path = path_prefix + "my series scorings/" + fileName + ".xml";
	QFile file(path + "_new");
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	
	QXmlStreamWriter stream(&file);
	stream.setAutoFormatting(true);
	stream.writeStartDocument();
	stream.writeStartElement("SeriesScoring");
	stream.writeAttribute("installationKey", installationKey);
	stream.writeAttribute("fileFormatVersion", "1");
	
	stream.writeEmptyElement("Settings");
	stream.writeAttribute("count_only_best_runs", count_only_best_runs ? "yes" : "no");
	stream.writeAttribute("num_counting_runs", QString::number(num_counting_runs));
	
	stream.writeEmptyElement("OrganizerBonus");
	stream.writeAttribute("organizer_bonus_type", QString::number(organizer_bonus_type));
	stream.writeAttribute("organizer_bonus_fixed", QString::number(organizer_bonus_fixed.toInt()));
	stream.writeAttribute("organizer_bonus_percentage", QString::number(organizer_bonus_percentage.toInt()));
	stream.writeAttribute("organizer_bonus_counting_runs", QString::number(organizer_bonus_counting_runs));
	
	for (std::vector<AbstractCategory*>::iterator it = categories.begin(); it != categories.end(); ++it)
	{
		stream.writeEmptyElement("Category");
		stream.writeAttribute("name", (*it)->name);
		stream.writeAttribute("number", QString::number((*it)->number));
	}
	
	for (ResultMap::iterator it = resultMap.begin(); it != resultMap.end(); ++it)
	{
		stream.writeStartElement("RaceResult");
		stream.writeAttribute("number", QString::number(it->first));
		stream.writeAttribute("name", it->second->name);
		
		for (std::set< Runner* >::iterator rit = it->second->organizers.begin(); rit != it->second->organizers.end(); ++rit)
		{
			Runner* runner = *rit;
			runner->saveToFile(&stream, false);
		}
		
		it->second->list->saveToFile(&stream);
		
		stream.writeEndElement();
	}
	
	stream.writeStartElement("EndResult");
	stream.writeAttribute("dirty", result_dirty ? "yes" : "no");
	if (result)
		result->saveToFile(&stream);
	stream.writeEndElement();
	
	stream.writeEndElement();
	stream.writeEndDocument();
	
	QFile::remove(path);
	file.rename(path);
	
	return true;
}
bool SeriesScoring::loadFromFile(QWidget* dialogParent)
{
	QFile file("my series scorings/" + fileName + ".xml");
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	
	ProblemWidget* problemWidget = new ProblemWidget();
	
	SeriesRaceResult* current_result = nullptr;
	int current_result_number;
	bool use_ids = false;
	
	QXmlStreamReader stream(&file);
	while (!stream.atEnd())
	{
		stream.readNext();
		if (stream.tokenType() != QXmlStreamReader::StartElement)
			continue;
		
		if (stream.name() == "Runner")
		{
			assert(current_result);
			
			if (use_ids)
			{
				Runner* runner = runnerDB.getByID(stream.attributes().value("id").toString().toInt());
				if (runner)
					current_result->organizers.insert(runner);
			}
			else
			{
				QString first_name = stream.attributes().value("first_name").toString();
				QString last_name = stream.attributes().value("last_name").toString();
				int year = stream.attributes().value("year").toString().toInt();
				bool isMale = stream.attributes().value("male") == "yes";
				
				Runner* runner = runnerDB.findRunner(first_name, last_name, year, isMale);
				if (!runner)
					problemWidget->addProblem(new OrganizerMissingProblem(current_result, first_name, last_name, year, isMale));
				else
					current_result->organizers.insert(runner);
			}
		}
		else if (stream.name() == "Category")
		{
			AbstractCategory* new_category = new AbstractCategory();
			new_category->name = stream.attributes().value("name").toString();
			new_category->number = stream.attributes().value("number").toString().toInt();
			categories.push_back(new_category);
		}
		else if (stream.name() == "ResultList")
		{
			ResultList* resultList = new ResultList(&stream, use_ids, this);
			if (current_result)
				current_result->list = resultList;
			else
				result = resultList;
		}
		else if (stream.name() == "RaceResult")
		{
			if (current_result)
				resultMap.insert(std::make_pair(current_result_number, current_result));
			current_result = new SeriesRaceResult();
			
			current_result_number = stream.attributes().value("number").toString().toInt();
			current_result->name = stream.attributes().value("name").toString();
		}
		else if (stream.name() == "EndResult")
		{
			if (current_result)
				resultMap.insert(std::make_pair(current_result_number, current_result));
			current_result = nullptr;
			result_dirty = stream.attributes().value("dirty") == "yes";
		}
		else if (stream.name() == "Settings")
		{
			count_only_best_runs = stream.attributes().value("count_only_best_runs") == "yes";
			num_counting_runs = stream.attributes().value("num_counting_runs").toString().toInt();
		}
		else if (stream.name() == "OrganizerBonus")
		{
			organizer_bonus_type = stream.attributes().value("organizer_bonus_type").toString().toInt();
			organizer_bonus_fixed = FPNumber(stream.attributes().value("organizer_bonus_fixed").toString().toInt());
			organizer_bonus_percentage = FPNumber(stream.attributes().value("organizer_bonus_percentage").toString().toInt());
			organizer_bonus_counting_runs = stream.attributes().value("organizer_bonus_counting_runs").toString().toInt();
		}
		else if (stream.name() == "SeriesScoring")
		{
			if (stream.attributes().value("installationKey").toString().compare(installationKey) == 0)
				use_ids = true;
		}
	}
	
	if (current_result)
		resultMap.insert(std::make_pair(current_result_number, current_result));
	
	if (problemWidget->problemCount() > 0)
		problemWidget->showAsDialog(tr("Problems while loading series scoring"), dialogParent);
	else
		delete problemWidget;
	
	return true;
}

void SeriesScoring::recalculateResult()
{
	result_dirty = false;
	delete result;
	ResultMap::iterator it = resultMap.begin();
	if (it == resultMap.end())
	{
		// No single result exists ...
		result = nullptr;
		return;
	}
	
	// Calculate the number of decimal places as the maximum from the result lists ...
	int decimal_places = it->second->list->getDecimalPlaces();
	int decimal_factor = it->second->list->getDecimalFactor();
	for (it = resultMap.begin(); it != resultMap.end(); ++it)
	{
		if (it->second->list->getDecimalPlaces() > decimal_places)
		{
			decimal_places = it->second->list->getDecimalPlaces();
			decimal_factor = it->second->list->getDecimalFactor();
		}
	}
	result = new ResultList(fileName, decimal_places);
	
	int colPointsStart;	// the number of the first point column
	
	// Look at the layout of the first result list to determine the aggregated layout ...
	it = resultMap.begin();
	bool individual_runner_results = it->second->list->getFirstRunnerColumn() != -1;
	if (individual_runner_results)
	{
		// Individual runners
		int colCategory = result->addColumn(ResultList::ColumnCategory);
		int colRank = result->addColumn(ResultList::ColumnRank);
		int colRunner = result->addColumn(ResultList::ColumnRunner);
		int colClub = result->addColumn(ResultList::ColumnClub);
		colPointsStart = colClub + 1;
		
		// Go through every race and add all single race results to the main result list
		std::map< std::pair<AbstractCategory*, Runner*>, int> runnerToRow;
		
		for (it = resultMap.begin(); it != resultMap.end(); ++it)
		{
			ResultList* source = it->second->list;
			int colPoints = result->addColumn(ResultList::ColumnPoints, it->second->name);
			int srcCategory = source->getCategoryColumn();
			int srcRunner = source->getFirstRunnerColumn();
			int srcClub = source->getClubColumn();
			int srcPoints = source->getPointsColumn();
			
			int size = source->rowCount();
			for (int i = 0; i < size; ++i)
			{
				if (!source->getData(i, srcPoints).isValid())
					continue;	// no need to do anything, as this is the default anyway, and runners which only have "-" results should not be added to the results list
				
				AbstractCategory* cat = static_cast<AbstractCategory*>(source->getData(i, srcCategory).value<void*>());
				Runner* runner = static_cast<Runner*>(source->getData(i, srcRunner).value<void*>());
				std::pair<AbstractCategory*, Runner*> key = std::make_pair(cat, runner);
				
				std::map< std::pair<AbstractCategory*, Runner*>, int>::iterator it = runnerToRow.find(key);
				int row;
				if (it == runnerToRow.end())
				{
					row = result->addRow();
					result->setData(row, colCategory, qVariantFromValue<void*>(cat));
					result->setData(row, colRank, QVariant(-1));
					result->setData(row, colRunner, qVariantFromValue<void*>(runner));
					result->setData(row, colClub, source->getData(i, srcClub));
					
					runnerToRow.insert(std::make_pair(key, row));
				}
				else
					row = it->second;
				
				result->setData(row, colPoints, static_cast<int>((source->getData(i, srcPoints).toInt() / (double)source->getDecimalFactor()) * decimal_factor + 0.5));
			}
		}
		
		if (organizer_bonus_type != NoOrganizerBonus && !runnerToRow.empty())
		{
			// Calculate organizer bonus
			// NOTE: If an organizer already has a result, disregard it because it is assumed to not count
			// NOTE: If an organizer did otherwise not participate in the series scoring, (s)he will not get bonus points if
			//       there are multiple categories because the category to give the points is not known
			
			// Check if there is only one category
			bool only_one_category = true;
			AbstractCategory* onlyCategory = runnerToRow.begin()->first.first;
			for (std::map< std::pair<AbstractCategory*, Runner*>, int>::iterator runmapit = runnerToRow.begin(); runmapit != runnerToRow.end(); ++runmapit)
			{
				if (runmapit->first.first != onlyCategory)
				{
					only_one_category = false;
					onlyCategory = nullptr;
					break;
				}
			}
			
			// For every race ...
			int column = colPointsStart;
			for (it = resultMap.begin(); it != resultMap.end(); ++it)
			{
				// For every organizer ...
				for (std::set<Runner*>::iterator rit = it->second->organizers.begin(); rit != it->second->organizers.end(); ++rit)
				{
					// Give organizer bonus for every category the organizer has run in (TODO: should this be configurable to "only the category with the most / best results"?)
					bool bonus_given = false;
					
					// Give bonus points
					for (std::map< std::pair<AbstractCategory*, Runner*>, int>::iterator runmapit = runnerToRow.begin(); runmapit != runnerToRow.end(); ++runmapit)
					{
						if (runmapit->first.second == *rit)
						{
							giveOrganizerBonus(runmapit->second, column, colPointsStart, result);
							bonus_given = true;
						}
					}
					
					// No bonus given and there is only one category? In this case, give the points for this category. Otherwise, we do not know the right one
					if (!bonus_given && only_one_category)
					{
						int row = result->addRow();
						result->setData(row, colCategory, qVariantFromValue<void*>(onlyCategory));
						result->setData(row, colRank, QVariant(-1));
						result->setData(row, colRunner, qVariantFromValue<void*>(*rit));
						result->setData(row, colClub, qVariantFromValue<void*>(((*rit)->getNumClubs() > 0) ? (*rit)->getClub(0) : nullptr));	// NOTE: here, we just take the first club. Should not be a mayor problem as this is a corner case anyway
						
						runnerToRow.insert(std::make_pair(std::make_pair(onlyCategory, *rit), row));
						giveOrganizerBonus(row, column, colPointsStart, result);
					}
				}
				
				++column;
			}
		}
	}
	else
	{
		// Clubs
		int colRank = result->addColumn(ResultList::ColumnRank);
		int colClub = result->addColumn(ResultList::ColumnClub);
		colPointsStart = colClub + 1;
		
		// Go through every race and add all single race results to the main result list
		std::map<Club*, int> clubToRow;
		
		for (it = resultMap.begin(); it != resultMap.end(); ++it)
		{
			ResultList* source = it->second->list;
			int colAdditionalStart = -1;
			if (source->getAdditionalStartColumn() >= 0)
			{
				ResultList::ColumnType type = source->getColumnType(source->getAdditionalStartColumn());
				colAdditionalStart = result->addColumn((type == ResultList::ColumnPoints) ? ResultList::ColumnPointInfo : type, source->getColumnLabel(source->getAdditionalStartColumn()));
				
				for (int i = source->getAdditionalStartColumn() + 1; i <= source->getAdditionalEndColumn(); ++i)
				{
					type = source->getColumnType(i);
					result->addColumn((type == ResultList::ColumnPoints) ? ResultList::ColumnPointInfo : type, source->getColumnLabel(i));
				}
			}
			int colPoints = result->addColumn(ResultList::ColumnPoints, it->second->name);
			int srcClub = source->getClubColumn();
			int srcPoints = source->getPointsColumn();
			
			int size = source->rowCount();
			for (int i = 0; i < size; ++i)
			{
				Club* club = static_cast<Club*>(source->getData(i, srcClub).value<void*>());
				
				std::map<Club*, int>::iterator cit = clubToRow.find(club);
				
				int row;
				if (cit == clubToRow.end())
				{
					row = result->addRow();
					result->setData(row, colRank, QVariant(-1));
					result->setData(row, colClub, qVariantFromValue<void*>(club));
					
					clubToRow.insert(std::make_pair(club, row));
				}
				else
					row = cit->second;
				
				result->setData(row, colPoints, static_cast<int>((source->getData(i, srcPoints).toInt() / (double)source->getDecimalFactor()) * decimal_factor + 0.5));
				if (colAdditionalStart >= 0)
				{
					for (int k = source->getAdditionalStartColumn(); k <= source->getAdditionalEndColumn(); ++k)
					{
						if (source->getColumnType(k) == ResultList::ColumnPointInfo || source->getColumnType(k) == ResultList::ColumnPoints)
						{
							if (source->getData(i, k).isValid())
								result->setData(row, colAdditionalStart + k - source->getAdditionalStartColumn(), static_cast<int>((source->getData(i, k).toInt() / (double)source->getDecimalFactor()) * decimal_factor + 0.5));
							else
								result->setData(row, colAdditionalStart + k - source->getAdditionalStartColumn(), QVariant());
						}
						else
							result->setData(row, colAdditionalStart + k - source->getAdditionalStartColumn(), source->getData(i, k));
					}
				}
			}
		}
	}
	
	// Calculate aggregated results column
	int colResultingPoints = result->addColumn(ResultList::ColumnPoints, tr("Resulting points"));
	
	int size = result->rowCount();
	for (int row = 0; row < size; ++row)	// for every row ...
	{
		std::multiset<double, std::greater<double> > best_points;
		for (int i = colPointsStart; i < colResultingPoints; ++i)
		{
			if (result->getColumnType(i) != ResultList::ColumnPoints)
				continue;
			
			int points = result->getData(row, i).toInt();
			if (points < 0)
				points = -points;
			
			best_points.insert(points / (double)result->getDecimalFactor());
		}
		
		double resulting_points = 0;
		int num_results = 0;
		std::multiset<double, std::greater<double> >::iterator it = best_points.begin();
		while ((!count_only_best_runs || num_results < num_counting_runs) && it != best_points.end())
		{
			resulting_points += *it;
			++num_results;
			++it;
		}
		
		result->setData(row, colResultingPoints, static_cast<int>(resulting_points * result->getDecimalFactor() + 0.5));
	}
	
	// Sort results
	result->calculateRanks(colResultingPoints, -1, false);
}

void SeriesScoring::giveOrganizerBonus(int row, int column, int first_point_column, ResultList* result)
{
	double bonus;
	
	if (organizer_bonus_type == FixedOrganizerBonus)
		bonus = organizer_bonus_fixed.toDouble();
	else if (organizer_bonus_type == PercentageOrganizerBonus)
	{
		std::multiset<double, std::greater<double> > best_points;
		for (int i = first_point_column; i < result->columnCount(); ++i)
		{
			if (i == column)
				continue;
			
			int points = result->getData(row, i).toInt();
			if (points < 0)
				continue;	// Do not use the bonus for one race to calculate the bonus for another race
				
			best_points.insert(points / result->getDecimalFactor());
		}
		
		bonus = 0;
		int num_results = 0;
		std::multiset<double, std::greater<double> >::iterator it = best_points.begin();
		while (num_results < organizer_bonus_counting_runs && it != best_points.end())
		{
			bonus += *it;
			++num_results;
			++it;
		}
		if (num_results > 0)
			bonus = (bonus / num_results) * (organizer_bonus_percentage.toDouble() / 100);
	}
	else
		assert(false);
	
	result->setData(row, column, -(static_cast<int>(bonus * result->getDecimalFactor() + 0.5)));
}

void SeriesScoring::mergeClubs(Club* src, Club* dest)
{
	if (result && result->getClubColumn() >= 0)
		result->changeInColumn(result->getClubColumn(), qVariantFromValue<void*>(src), qVariantFromValue<void*>(dest));
	for (ResultMap::iterator it = resultMap.begin(); it != resultMap.end(); ++it)
	{
		ResultList* list = it->second->list;
		if (list->getClubColumn() >= 0)
			list->changeInColumn(list->getClubColumn(), qVariantFromValue<void*>(src), qVariantFromValue<void*>(dest));
	}
}
void SeriesScoring::mergeRunners(Runner* src, Runner* dest)
{
	if (result && result->getFirstRunnerColumn() >= 0)
		for (int i = result->getFirstRunnerColumn(); i <= result->getLastRunnerColumn(); ++i)
			result->changeInColumn(i, qVariantFromValue<void*>(src), qVariantFromValue<void*>(dest));
	for (ResultMap::iterator it = resultMap.begin(); it != resultMap.end(); ++it)
	{
		ResultList* list = it->second->list;
		if (list->getFirstRunnerColumn() >= 0)
			for (int i = list->getFirstRunnerColumn(); i <= list->getLastRunnerColumn(); ++i)
				list->changeInColumn(i, qVariantFromValue<void*>(src), qVariantFromValue<void*>(dest));
			
		std::set<Runner*>::iterator r_it = it->second->organizers.find(src);
		if (r_it != it->second->organizers.end())
		{
			it->second->organizers.erase(r_it);
			it->second->organizers.insert(dest);
		}
	}
}

AbstractCategory* SeriesScoring::findOrAddCategory(const QString& name, int number)
{
	for (std::vector< AbstractCategory* >::iterator it = categories.begin(); it != categories.end(); ++it)
	{
		if ((*it)->name.compare(name, Qt::CaseInsensitive) == 0)
			return *it;
	}
	
	AbstractCategory* new_category = new AbstractCategory();
	new_category->name = name;
	new_category->number = number;
	categories.push_back(new_category);
	
	return new_category;
}
AbstractCategory* SeriesScoring::findInternCategory(const QString& name)
{
	for (std::vector< AbstractCategory* >::iterator it = categories.begin(); it != categories.end(); ++it)
	{
		if ((*it)->name.compare(name, Qt::CaseInsensitive) == 0)
			return *it;
	}
	
	return nullptr;
}
void SeriesScoring::cleanupCategories()
{
	int numCategories = categories.size();
	int numUsedCategories = 0;
	bool* usedCategories = new bool[numCategories];
	memset(usedCategories, 0, sizeof(bool) * numCategories);
	
	for (ResultMap::iterator it = resultMap.begin(); it != resultMap.end(); ++it)
	{
		ResultList* list = it->second->list;
		
		int colCategory = list->getCategoryColumn();
		if (colCategory == -1)
			continue;
		
		int num_rows = list->rowCount();
		for (int i = 0; i < num_rows; ++i)
		{
			AbstractCategory* existingCategory = reinterpret_cast<AbstractCategory*>(list->getData(i, colCategory).value<void*>());
			for (int k = 0; k < numCategories; ++k)
			{
				if (categories[k] != existingCategory)
					continue;
				if (usedCategories[k])
					break;
				
				++numUsedCategories;
				usedCategories[k] = true;
				if (numUsedCategories == numCategories)
					return;	// all categories still there
			}
		}
	}
	
	int offset = 0;
	for (int k = 0; k < numCategories; ++k)
	{
		if (usedCategories[k])
			continue;
		
		delete categories[k - offset];
		categories.erase(categories.begin() + (k - offset));
		++offset;
	}
	
	delete[] usedCategories;
}

SeriesRaceResult* SeriesScoring::addResult(int run_number, const QString& name, ResultList* list, const std::set< Runner* >& organizers)
{
	SeriesRaceResult* new_result = new SeriesRaceResult();
	new_result->name = name;
	new_result->list = new ResultList(*list, true);
	new_result->organizers = organizers;
	
	ResultMap::iterator it = resultMap.find(run_number);
	if (it != resultMap.end())
	{
		delete it->second;
		resultMap.erase(it);
		cleanupCategories();
	}
	resultMap.insert(ResultMap::value_type(run_number, new_result));
	
	int colCategory = new_result->list->getCategoryColumn();
	if (colCategory != -1)
	{
		// Add new categories and change category pointers to the categories stored in the SeriesScoring
		int num_rows = new_result->list->rowCount();
		for (int i = 0; i < num_rows; ++i)
		{
			AbstractCategory* oldCategory = reinterpret_cast<AbstractCategory*>(new_result->list->getData(i, colCategory).value<void*>());
			AbstractCategory* newCategory = findOrAddCategory(oldCategory->name, oldCategory->number);
			new_result->list->setData(i, colCategory, qVariantFromValue<void*>(newCategory));
		}
	}
	
	result_dirty = true;
	return new_result;
}
void SeriesScoring::deleteResult(int run_number)
{
	ResultMap::iterator it = resultMap.find(run_number);
	assert(it != resultMap.end());
	
	delete it->second;
	resultMap.erase(it);
	cleanupCategories();
	result_dirty = true;
}
void SeriesScoring::deleteResult(SeriesRaceResult* result)
{
	for (ResultMap::iterator it = resultMap.begin(); it != resultMap.end(); ++it)
	{
		if (it->second == result)
		{
			delete it->second;
			resultMap.erase(it);
			cleanupCategories();
			result_dirty = true;
			return;
		}
	}
	assert(false);
}
void SeriesScoring::changeResultNumber(int src, int dest)
{
	ResultMap::iterator it = resultMap.find(src);
	assert(it != resultMap.end());
	
	SeriesRaceResult* race_result = it->second;
	resultMap.erase(it);
	bool success = resultMap.insert(std::make_pair(dest, race_result)).second;
	assert(success);
	
	result_dirty = true;
}
bool SeriesScoring::hasResult(int run_number)
{
	return resultMap.find(run_number) != resultMap.end();
}
int SeriesScoring::getRaceResultNumber(SeriesRaceResult* result)
{
	for (ResultMap::iterator it = resultMap.begin(); it != resultMap.end(); ++it)
	{
		if (it->second == result)
			return it->first;
	}
	assert(false);
	return 0;
}

bool SeriesScoring::referencesRunner(Runner* runner)
{
	for (ResultMap::iterator it = resultMap.begin(); it != resultMap.end(); ++it)
	{
		SeriesRaceResult* race = it->second;
		if (race->organizers.find(runner) != race->organizers.end())
			return true;
		if (race->list)
		{
			for (int i = race->list->getFirstRunnerColumn(); i <= race->list->getLastRunnerColumn(); ++i)
				if (race->list->findInColumn(i, qVariantFromValue<void*>(runner)) != -1)
					return true;
		}
	}
	return false;
}

// ### SeriesScoringDB ###

SeriesScoringDB::SeriesScoringDB()
{
	const QString dir_name = "my series scorings";
	
	QDir dir(dir_name);
	if (!dir.exists())
	{
		QDir curDir;
		curDir.mkdir(dir_name);
		dir = QDir(dir_name);
	}
	QStringList nameFilters;
	nameFilters << ("*.xml");
	QStringList myScorings = dir.entryList(nameFilters, QDir::Files | QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::Name);
	
	for (int i = 0; i < myScorings.size(); ++i)
		scorings.insert(myScorings.at(i).left(myScorings.at(i).size() - 4), nullptr);
	
	// TODO: dir watcher
}

SeriesScoring* SeriesScoringDB::getOrLoadScoring(const QString& name, QWidget* dialogParent)
{
	Scorings::iterator it = scorings.find(name);
	if (it == scorings.end())
		return nullptr;
	
	if (*it)
		return *it;
	
	// Scoring not loaded yet
	SeriesScoring* loadedScoring = new SeriesScoring(name);
	if (!loadedScoring->loadFromFile(dialogParent))
	{
		delete loadedScoring;
		return nullptr;
	}
	scorings.insert(name, loadedScoring);
	
	return loadedScoring;
}

bool SeriesScoringDB::saveToFile(const QString path_prefix)
{
	for (Scorings::iterator it = scorings.begin(); it != scorings.end(); ++it)
	{
		if (!*it)
			continue;
		
		(*it)->saveToFile(path_prefix);
	}
	return true;
}

void SeriesScoringDB::mergeClubs(Club* src, Club* dest)
{
	for (Scorings::iterator it = scorings.begin(); it != scorings.end(); ++it)
	{
		if (!*it)
			continue;
		
		(*it)->mergeClubs(src, dest);
	}
}
void SeriesScoringDB::mergeRunners(Runner* src, Runner* dest)
{
	for (Scorings::iterator it = scorings.begin(); it != scorings.end(); ++it)
	{
		if (!*it)
			continue;
		
		(*it)->mergeRunners(src, dest);
	}
}

SeriesScoring* SeriesScoringDB::createNewScoring(const QString& desiredName)
{
	QString name = getUnusedName(desiredName);
	SeriesScoring* new_scoring = new SeriesScoring(name);
	scorings.insert(name, new_scoring);
	return new_scoring;
}

void SeriesScoringDB::changeScoringName(const QString& old_name, const QString& new_name)
{
	assert(old_name != new_name);
	Scorings::iterator it = scorings.find(old_name);
	if (it == scorings.end())
		return;
	
	scorings.insert(new_name, it.value());
	scorings.erase(it);
	QFile::rename("my series scorings/" + old_name + ".xml", "my series scorings/" + new_name + ".xml");
}

void SeriesScoringDB::deleteScoring(const QString& name)
{
	scorings.remove(name);
	QFile::remove("my series scorings/" + name + ".xml");
}

bool SeriesScoringDB::openScoringReferencesRunner(Runner* runner)
{
	for (Scorings::iterator it = scorings.begin(); it != scorings.end(); ++it)
	{
		if (!*it)
			continue;
		
		if ((*it)->referencesRunner(runner))
			return true;
	}
	return false;
}

QString SeriesScoringDB::getUnusedName(const QString& desiredName)
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

// ### OrganizerMissingProblem ###

OrganizerMissingProblem::OrganizerMissingProblem(SeriesRaceResult* result, const QString& first_name, const QString& last_name, int year, bool isMale)
: result(result), first_name(first_name), last_name(last_name), year(year), isMale(isMale)
{
	description = tr("Cannot find organizer %1 %2 in runner database").arg(first_name).arg(last_name);
	// TODO: check for similar names in runner DB and offer to change reference to them
}
int OrganizerMissingProblem::getNumSolutions()
{
	return 2;
}
QString OrganizerMissingProblem::getSolutionDescription(int i)
{
	if (i == 0) return tr("Add to database");
	else return tr("Omit organizer");
}
void OrganizerMissingProblem::applySolution(int i)
{
	if (i == 0)
	{
		Runner* runner = new Runner();
		runner->setFirstName(first_name);
		runner->setLastName(last_name);
		runner->setYear(year);
		runner->setIsMale(isMale);
		runnerDB.addRunner(runner);
		result->organizers.insert(runner);
	}
}
