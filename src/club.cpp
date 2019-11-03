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


#include "club.h"

#include "assert.h"

#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QFile>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include "location.h"
#include "util.h"
#include "config.h"
#include "runner.h"
#include "scoring.h"
#include "seriesScoring.h"

const int COLUMN_COUNT = 3;

// ### Club ###

Club::Club(const QString& _name) : name(_name), country(nullptr), province(nullptr)
{
}
Club::~Club()
{
}

// ### ClubDB ###

ClubDB::ClubDB()
{
	nextID = 0;
}
ClubDB::~ClubDB()
{
	size_t size = clubs.size();
	for (size_t i = 0; i < size; ++i)
		delete clubs[i];
}

bool ClubDB::saveToFile(const QString path_prefix)
{
	QString path = path_prefix + "databases/clubs.xml";
	QFile file(path + "_new");
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	
	QXmlStreamWriter stream(&file);
	stream.setAutoFormatting(true);
	stream.writeStartDocument();
	stream.writeStartElement("Clubs");
	stream.writeAttribute("installationKey", installationKey);
	stream.writeAttribute("fileFormatVersion", "1");
	
	for (std::map<int, int>::iterator it = merge_info.begin(); it != merge_info.end(); ++it)
	{
		stream.writeEmptyElement("Merge");
		stream.writeAttribute("from", QString::number(it->first));
		stream.writeAttribute("to", QString::number(it->second));
	}
	
	int size = clubs.size();
	for (int i = 0; i < size; ++i)
	{
		Club* club = clubs[i];
		
		stream.writeEmptyElement("Club");
		stream.writeAttribute("name", club->getName());
		stream.writeAttribute("id", QString::number(club->getID()));
		if (club->getCountry())
			stream.writeAttribute("country", club->getCountry()->getName());
		if (club->getProvince())
			stream.writeAttribute("region", club->getProvince()->getName());
	}
	
	stream.writeEndElement();
	stream.writeEndDocument();
	
	QFile::remove(path);
	file.rename(path);
	
	return true;
}
bool ClubDB::loadFromFile()
{
	QFile file("databases/clubs.xml");
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	
	QXmlStreamReader stream(&file);
	while (!stream.atEnd())
	{
		stream.readNext();
		if (stream.tokenType() != QXmlStreamReader::StartElement)
			continue;
		
		if (stream.name() == "Club")
		{
			Club* new_club = new Club(stream.attributes().value("name").toString());
			Location* country = nullptr;
			Location* province = nullptr;
			
			QStringRef ref = stream.attributes().value("country");
			if (!ref.isEmpty())
			{
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
			
			new_club->setLocation(country, province);
			new_club->setID(stream.attributes().value("id").toString().toInt());
			addClub(new_club, false);
		}
		else if (stream.name() == "Merge")
			merge_info.insert(std::make_pair(stream.attributes().value("from").toString().toInt(), stream.attributes().value("to").toString().toInt()));
	}
	
	return true;
}

int ClubDB::getFreeID()
{
	while (id_map.find(nextID) != id_map.end())
		++nextID;
	++nextID;
	return nextID - 1;
}
Club* ClubDB::getByID(int id)
{
	std::map<int, int>::iterator it = merge_info.find(id);
	while (it != merge_info.end())
	{
		id = it->second;
		it = merge_info.find(id);
	}
	
	std::map<int, Club*>::iterator id_it = id_map.find(id);
	if (id_it == id_map.end())
		return nullptr;
	else
		return id_it->second;
}

bool ClubDB::containsName(const QString& name, Club* exclude)
{
	size_t end = clubs.size();
	for (size_t i = 0; i < end; ++i)
	{
		if ((clubs[i] != exclude) && (clubs[i]->getName().compare(name, Qt::CaseInsensitive) == 0))
			return true;
	}
	
	return false;
}
Club* ClubDB::findClub(const QString& name)
{
	size_t end = clubs.size();
	for (size_t i = 0; i < end; ++i)
	{
		if (clubs[i]->getName().compare(name, Qt::CaseInsensitive) == 0)
			return clubs[i];
	}
	
	return nullptr;
}
void ClubDB::findSimilarClubs(QString name, std::vector< Club* >& out)
{
	size_t end = clubs.size();
	for (size_t i = 0; i < end; ++i)
	{
		if (levenshteinDistanceSmaller(clubs[i]->getName(), name, 2))
			out.push_back(clubs[i]);
	}
}
Club* ClubDB::findClubAt(Location* location, Club* exclude)
{
	size_t end = clubs.size();
	for (size_t i = 0; i < end; ++i)
	{
		if ((clubs[i] != exclude) && ((clubs[i]->getCountry() == location) || (clubs[i]->getProvince() == location)))
			return clubs[i];
	}
	
	return nullptr;
}
QString ClubDB::getUnusedName(const QString& desiredName)
{
	if (!containsName(desiredName))
		return desiredName;
	
	for (int i = 2; i < 9999; ++i)
	{
		QString name = desiredName + " #" + QString::number(i);
		if (!containsName(name))
			return name;
	}
	
	// Should practically never happen
	return desiredName;
}

void ClubDB::addClub(Club* club, bool setNewID)
{
	assert(findClub(club->getName()) == nullptr && "Tried to add a club whose name is already used by another club!");
	
	int row = static_cast<int>(clubs.size());
	int count = 1;
	
	beginInsertRows(QModelIndex(), row, row + count - 1);
	if (setNewID)
		club->setID(getFreeID());
	id_map.insert(std::make_pair(club->getID(), club));
	clubs.push_back(club);
	endInsertRows();
}
void ClubDB::deleteClub(Club* club)
{
	size_t end = clubs.size();
	for (size_t i = 0; i < end; ++i)
	{
		if (clubs[i] == club)
		{
			removeRow(i);
			return;
		}
	}
}
void ClubDB::mergeClubs(Club* src, Club* dest)
{
	merge_info.insert(std::make_pair(src->getID(), dest->getID()));
	runnerDB.changeClubForAllRunners(src, dest);
	scoringDB.mergeClubs(src, dest);
	seriesScoringDB.mergeClubs(src, dest);
	clubDB.deleteClub(src);
}
Club* ClubDB::getItem(const QModelIndex& index) const
{
	if (index.isValid())
	{
		Club* item = static_cast<Club*>(index.internalPointer());
		if (item)
			return item;
	}
	
	return nullptr;
}

int ClubDB::rowCount(const QModelIndex& /*parent*/) const
{
	return clubs.size();
}
int ClubDB::columnCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;	// the docs say that when implementing a table based model, columnCount() should return 0 when the parent is valid
	else
		return COLUMN_COUNT;
}
QModelIndex ClubDB::index(int row, int column, const QModelIndex& parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();
	if (row < 0 || row >= static_cast<int>(clubs.size()))
		return QModelIndex();
	if (column < 0 || column >= COLUMN_COUNT)
		return QModelIndex();
	
	return createIndex(row, column, clubs[row]);
}
QVariant ClubDB::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role != Qt::DisplayRole && role != Qt::EditRole)
		return QVariant();
	
	Club* item = getItem(index);
	if (!item)
		return QVariant();
	
	if (index.column() == 0)
		return item->getName();
	else if (index.column() == 1)
		return item->getCountry() ? item->getCountry()->getName() : "";
	else if (index.column() == 2)
		return item->getProvince() ? item->getProvince()->getName() : "";
	else
		return QVariant();
}
QVariant ClubDB::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		if (section == 0)
			return QVariant(tr("Name"));
		else if (section == 1)
			return QVariant(tr("Country"));	
		else if (section == 2)
			return QVariant(tr("State/Province"));
	}
	
	return QVariant();
}
Qt::ItemFlags ClubDB::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return {};

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool ClubDB::insertRows(int row, int count, const QModelIndex& parent)
{
	if (row < 0 || row > static_cast<int>(clubs.size()))
		return false;
	
	beginInsertRows(parent, row, row + count - 1);
	for (int r = 0; r < count; ++r)
	{
		Club* new_club = new Club(getUnusedName(tr("New club")));
		new_club->setID(getFreeID());
		clubs.insert(clubs.begin() + row, new_club);
	}
	endInsertRows();
	
	return true;
}
bool ClubDB::removeRows(int row, int count, const QModelIndex& parent)
{
	if (row < 0 || row + count > static_cast<int>(clubs.size()))
		return false;
	
	beginRemoveRows(parent, row, row + count - 1);
	for (int r = row; r < row + count; ++r)
	{
		id_map.erase(clubs[r]->getID());
		delete clubs[r];
		clubs.erase(clubs.begin() + r);
	}
	endRemoveRows();
	
	return true;
}

// ### ClubTable ###

ClubTable::ClubTable()
{
	sortedModel = new QSortFilterProxyModel();
	sortedModel->setSourceModel(&clubDB);
	sortedModel->setDynamicSortFilter(true);
	sortedModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	setModel(sortedModel);
	setSortingEnabled(true);
	sortByColumn(0, Qt::AscendingOrder);
	
	setSelectionBehavior(QAbstractItemView::SelectRows);
	verticalHeader()->setVisible(false);
	
	resizeColumnsToContents();
	QHeaderView* headerView = horizontalHeader();
	headerView->setSectionResizeMode(0, QHeaderView::Stretch);
	headerView->setHighlightSections(false);
	
	//connect(&clubDB, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(sort()));
}
ClubTable::~ClubTable()
{
	setModel(nullptr);
	delete sortedModel;
}

void ClubTable::sort()
{
	sortedModel->sort(sortedModel->sortColumn());
}
