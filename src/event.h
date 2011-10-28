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


#ifndef EVENT_H
#define EVENT_H

#include <vector>

#include <QString>

class Scoring;
class ResultList;

/// Base class for run categories and custom scoring categories which can aggregate run categories
class AbstractCategory
{
public:
	
	int number;
	QString name;
};

/// TODO: unfinished (for OResults, only name was needed)
class Category : public AbstractCategory
{
};

/// The main class for events (*.oevt)
class Event
{
public:
	
	Event();
	~Event();
	
	// Result list
	void setResultList(ResultList* list);
	inline ResultList* getResultList() {return resultList;}
	
	// Category list
	void addCategory(Category* category);
	void removeCategory(int i);
	void clearCategories();
	int getNumCategories();
	Category* getCategory(int i);
	Category* findCategory(const QString& name);
	
	// Scoring list
	void addScoring(Scoring* scoring);
	void removeScoring(Scoring* scoring);
	int getNumScorings();
	Scoring* getScoring(int i);
	
	void calculateScorings(int event_year);
	void clearScoringLists();
	int getNumScoringLists();
	ResultList* getScoringList(int i);
	
private:
	
	typedef std::vector< Category* > Categories;
	typedef std::vector< Scoring* > Scorings;
	
	ResultList* resultList;
	std::vector< ResultList* > scoringLists;
	
	Categories categories;
	Scorings scorings;
};

#endif
