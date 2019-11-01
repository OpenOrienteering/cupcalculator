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


#include "location.h"

#include <assert.h>

#include <QSortFilterProxyModel>
#include <QFile>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include "global.h"

QSortFilterProxyModel* LocationDB::sortModel;

// ### Location ###

Location::Location(const QString& _name, Location* _parent) : parent(_parent), name(_name)
{
}
Location::~Location()
{
	Children::iterator it_end = children.end();
	for (Children::iterator it = children.begin(); it != it_end; ++it)
		delete *it;
}

Location* Location::findChild(const QString& name)
{
	Children::iterator it_end = children.end();
	for (Children::iterator it = children.begin(); it != it_end; ++it)
	{
		if ((*it)->getName().compare(name, Qt::CaseInsensitive) == 0)
			return *it;
	}
	return NULL;
}
Location* Location::getChild(int i)
{
	if (i < 0 || i >= static_cast<int>(children.size()))
		return NULL;
	return children[i];
}
size_t Location::getChildNumber()
{
	if (!parent)
		return 0;
	
	int i = 0;
	Children::iterator it_end = parent->children.end();
	for (Children::iterator it = parent->children.begin(); it != it_end; ++it)
	{
		if (*it == this)
			return i;
		++i;
	}
	
	return 0;
}

bool Location::insertChildren(int position, int count, int columns)
{
	if (position < 0 || position > static_cast<int>(children.size()))
		return false;
	
	for (int row = 0; row < count; ++row)
	{
		Location* item = new Location("", this);
		children.insert(children.begin() + position, item);
	}
	
    return true;
}
bool Location::removeChildren(int position, int count)
{
	if (position < 0 || position + count > static_cast<int>(children.size()))
		return false;
	
	for (int row = 0; row < count; ++row)
	{
		Children::iterator it = children.begin() + position;
		delete *it;
		children.erase(it);
	}
	
	return true;
}

// ### LocationDB ###

LocationDB::LocationDB() : QAbstractItemModel(NULL)
{
	nextID = 0;
	root_item = new Location("", NULL);
	
	sortModel = new QSortFilterProxyModel();
	sortModel->setSourceModel(this);
	
	sort();
	connect(this, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(sort()));
}
LocationDB::~LocationDB()
{
	delete root_item;
}

void LocationDB::sort()
{
	// NOTE: could debug this method for performance problems (is it called twice when it should be called just once?)
	sortModel->sort(0);
}

Location* LocationDB::getItem(const QModelIndex& index) const
{
	if (index.isValid())
	{
		Location* item = static_cast<Location*>(index.internalPointer());
		if (item)
			return item;
	}
	
	return root_item;
}

Location* LocationDB::getParent(Location* location) const
{
	if (location->getParent() == root_item)
		return NULL;
	else
		return location->getParent();
}

int LocationDB::rowCount(const QModelIndex& parent) const
{
	return getItem(parent)->getChildCount();
}
int LocationDB::columnCount(const QModelIndex& parent) const
{
	return 1;
}
Qt::ItemFlags LocationDB::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return 0;

	return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
QModelIndex LocationDB::index(int row, int column, const QModelIndex& parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();
	
	Location* parentItem = getItem(parent);
	
	Location* childItem = parentItem->getChild(row);
	if (childItem)
		return createIndex(row, column, childItem);
	else
		return QModelIndex();
}
QModelIndex LocationDB::parent(const QModelIndex& index) const
{
	if (!index.isValid())
		return QModelIndex();
	
	Location* childItem = getItem(index);
	Location* parentItem = childItem->getParent();
	
	if (parentItem == root_item)
		return QModelIndex();
	
	return createIndex(parentItem->getChildNumber(), 0, parentItem);
}
QVariant LocationDB::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();
	
	if (role != Qt::DisplayRole && role != Qt::EditRole)
		return QVariant();
	
	Location* item = getItem(index);
	return item->getName();
}
QVariant LocationDB::headerData(int section, Qt::Orientation orientation, int role) const
{
	//if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	//	return QVariant(tr("Country, state or province name"));
	
	return QVariant();
}

bool LocationDB::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (role != Qt::EditRole)
		return false;
	
	if (index.column() != 0)
		return false;
	
	Location* item = getItem(index);
	item->setName(value.toString());
	
	emit dataChanged(index, index);
	return true;
}
bool LocationDB::insertRows(int position, int rows, const QModelIndex& parent)
{
	Location* parentItem = getItem(parent);
	bool success;
	
	beginInsertRows(parent, position, position + rows - 1);
	success = parentItem->insertChildren(position, rows, 1);
	endInsertRows();
	
	return success;
}
bool LocationDB::removeRows(int position, int rows, const QModelIndex& parent)
{
	Location* parentItem = getItem(parent);
	bool success = true;
	
	beginRemoveRows(parent, position, position + rows - 1);
	for (int i = position; i < position + rows; ++i)
		id_map.erase(getItem(index(i, 0, parent))->getID());
	success = parentItem->removeChildren(position, rows);
	endRemoveRows();
	
	return success;
}

bool LocationDB::saveToFile(const QString path_prefix)
{
	QString path = path_prefix + "databases/locations.xml";
	QFile file(path + "_new");
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	
	QXmlStreamWriter stream(&file);
	stream.setAutoFormatting(true);
	stream.writeStartDocument();
	stream.writeStartElement("Locations");
	stream.writeAttribute("installationKey", installationKey);
	stream.writeAttribute("fileFormatVersion", "1");
	
	int size = root_item->getChildCount();
	for (int i = 0; i < size; ++i)
	{
		Location* country = root_item->getChild(i);
		
		int region_size = country->getChildCount();
		if (region_size > 0)
		{
			stream.writeStartElement("Country");
			stream.writeAttribute("name", country->getName());
			stream.writeAttribute("id", QString::number(country->getID()));
			for (int k = 0; k < region_size; ++k)
			{
				stream.writeEmptyElement("Region");
				stream.writeAttribute("name", country->getChild(k)->getName());
				stream.writeAttribute("id", QString::number(country->getChild(k)->getID()));
			}
			stream.writeEndElement();
		}
		else
		{
			stream.writeEmptyElement("Country");
			stream.writeAttribute("name", country->getName());
			stream.writeAttribute("id", QString::number(country->getID()));
		}
	}

	stream.writeEndElement();
	stream.writeEndDocument();
	
	QFile::remove(path);
	file.rename(path);
	
	return true;
}
bool LocationDB::loadFromFile()
{
	QFile file("databases/locations.xml");
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	
	disconnect(this, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(sort()));
	
	QXmlStreamReader stream(&file);
	
	QModelIndex last_country;
	int country_pos = rowCount();
	int region_pos = 0;
	while (!stream.atEnd())
	{
		stream.readNext();
		if (stream.tokenType() != QXmlStreamReader::StartElement)
			continue;
		
		if (stream.name() == "Country")
		{
			insertRow(country_pos);
			last_country = index(country_pos, 0, QModelIndex());
			setData(last_country, stream.attributes().value("name").toString(), Qt::EditRole);
			
			Location* location = getItem(last_country);
			location->setID(stream.attributes().value("id").toString().toInt());
			id_map.insert(std::make_pair(location->getID(), location));
			
			++country_pos;
			region_pos = 0;
		}
		else if (stream.name() == "Region")
		{
			insertRow(region_pos, last_country);
			QModelIndex region = index(region_pos, 0, last_country);
			setData(region, stream.attributes().value("name").toString(), Qt::EditRole);
			
			Location* location = getItem(region);
			location->setID(stream.attributes().value("id").toString().toInt());
			id_map.insert(std::make_pair(location->getID(), location));
			
			++region_pos;
		}
	}
	
	connect(this, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(sort()));
	sort();
	return true;
}

Location* LocationDB::findCountry(const QString& name)
{
	int size = root_item->getChildCount();
	for (int i = 0; i < size; ++i)
	{
		Location* country = root_item->getChild(i);
		if (country->getName().compare(name, Qt::CaseInsensitive) == 0)
			return country;
	}
	return NULL;
}
Location* LocationDB::addCountry(const QString& name)
{
	int country_pos = rowCount();
	insertRow(country_pos);
	QModelIndex new_index = index(country_pos, 0, QModelIndex());
	setData(new_index, name, Qt::EditRole);
	Location* location = getItem(new_index);
	location->setID(getFreeID());
	id_map.insert(std::make_pair(location->getID(), location));
	return location;
}
Location* LocationDB::addProvince(Location* country, const QString& name)
{
	int size = root_item->getChildCount();
	for (int i = 0; i < size; ++i)
	{
		Location* current_country = root_item->getChild(i);
		if (current_country == country)
		{
			QModelIndex parent = index(i, 0, QModelIndex());
			
			int province_pos = rowCount(parent);
			insertRow(province_pos, parent);
			QModelIndex new_index = index(province_pos, 0, parent);
			setData(new_index, name, Qt::EditRole);
			Location* location = getItem(new_index);
			location->setID(getFreeID());
			id_map.insert(std::make_pair(location->getID(), location));
			return location;
		}
	}
	assert(false);
	return NULL;
}

int LocationDB::getFreeID()
{
	while (id_map.find(nextID) != id_map.end())
		++nextID;
	++nextID;
	return nextID - 1;
}
Location* LocationDB::getByID(int id)
{
	std::map<int, Location*>::iterator it = id_map.find(id);
	if (it == id_map.end())
		return NULL;
	else
		return it->second;
}
