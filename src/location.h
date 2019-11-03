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


#ifndef CUPCALCULATOR_LOACTION_H
#define CUPCALCULATOR_LOACTION_H

#include <cstddef>
#include <map>
#include <vector>

#include <Qt>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariant>

class QSortFilterProxyModel;

/// Country or state/province
class Location
{
public:
	
	typedef std::vector< Location* > Children;
	
	Location(const QString& _name, Location* _parent);
	~Location();
	
	Location* findChild(const QString& name);
	Location* getChild(int i);
	size_t getChildNumber();
	
	bool insertChildren(int position, int count, int columns);
	bool removeChildren(int position, int count);
	
	inline Location* getParent() const {return parent;}
	inline size_t getChildCount() const {return children.size();}
	
	inline QString getName() const {return name;}
	inline void setName(const QString new_name) {name = new_name;}
	
	inline int getID() const {return id;}
	inline void setID(int i) {id = i;}
	
private:
	
	Location* parent;
	/// Countries can have children, for states/provinces this stays empty
	Children children;
	
	QString name;
	int id;
};

/// Keeps a tree of countries, states and provinces
class LocationDB : public QAbstractItemModel
{
Q_OBJECT
public:
	
	~LocationDB();
	
	bool saveToFile(const QString path_prefix = "");
	bool loadFromFile();
	
	Location* findCountry(const QString& name);
	Location* addCountry(const QString& name);
	Location* addProvince(Location* country, const QString& name);
	
	Location* getItem(const QModelIndex &index) const;
	Location* getParent(Location* location) const;
	
	inline QSortFilterProxyModel* getSortModel() const {return sortModel;}
	
	inline int getNextID() const {return nextID;}
	void setNextID(int i) {nextID = i;}
	int getFreeID();
	Location* getByID(int id);
	
	// Overwritten from QAbstractItemModel
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &index) const;

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

	virtual bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex());
	virtual bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex());
	
	static inline LocationDB& getSingleton()
	{
		static LocationDB instance;
		return instance;
	}
	
public slots:
	
	void sort();
	
private:
	
	LocationDB();
	
	int nextID;
	std::map<int, Location*> id_map;
	Location* root_item;
	
	static QSortFilterProxyModel* sortModel;
};

#define locationDB LocationDB::getSingleton()

#endif
