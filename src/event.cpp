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


#include "event.h"

#include <assert.h>

#include "resultList.h"
#include "scoring.h"

Event::Event()
{
	resultList = NULL;
}
Event::~Event()
{
	clearScoringLists();
	clearCategories();
	
	delete resultList;
}

void Event::setResultList(ResultList* list)
{
	delete resultList;
	resultList = list;
}

void Event::addCategory(Category* category)
{
	category->number = categories.size();
	categories.push_back(category);
}
void Event::removeCategory(int i)
{
	delete categories[i];
	categories.erase(categories.begin() + i);
}
void Event::clearCategories()
{	
	size_t size = categories.size();
	for (size_t i = 0; i < size; ++i)
		delete categories[i];
}
Category* Event::findCategory(const QString& name)
{
	size_t size = categories.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (categories[i]->name.compare(name, Qt::CaseSensitive) == 0)
			return categories[i];
	}
	return NULL;
}
int Event::getNumCategories()
{
	return categories.size();
}
Category* Event::getCategory(int i)
{
	return categories[i];
}

void Event::addScoring(Scoring* scoring)
{
	scorings.push_back(scoring);
}
void Event::removeScoring(Scoring* scoring)
{
	size_t size = scorings.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (scorings[i] == scoring)
		{
			scorings.erase(scorings.begin() + i);
			return;
		}
	}
}
int Event::getNumScorings()
{
	return scorings.size();
}
Scoring* Event::getScoring(int i)
{
	return scorings[i];
}

void Event::clearScoringLists()
{
	size_t size = scoringLists.size();
	for (size_t i = 0; i < size; ++i)
		delete scoringLists[i];
	scoringLists.clear();
}
void Event::calculateScorings(int event_year)
{
	clearScoringLists();
	
	size_t size = scorings.size();
	for (size_t i = 0; i < size; ++i)
		scorings[i]->calculateScoring(resultList, event_year, scoringLists, Scoring::IncludeAllRunners);
}
int Event::getNumScoringLists()
{
	return static_cast<int>(scoringLists.size());
}
ResultList* Event::getScoringList(int i)
{
	assert(i < getNumScoringLists());
	return scoringLists[i];
}
