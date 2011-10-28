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


#ifndef CLUB_H
#define CLUB_H

#include <QString>
#include <QAbstractTableModel>
#include <QTableView>
#include <vector>

class Location;
class QSortFilterProxyModel;

/// Stores club data
class Club
{
public:
	
	Club(const QString& _name);
	~Club();
	
	inline void setName(const QString& value) {name = value;}
	inline const QString& getName() const {return name;}
	
	inline void setLocation(Location* _country, Location* _province) {country = _country; province = _province;}
	inline Location* getCountry() const {return country;}
	inline Location* getProvince() const {return province;}
	
	inline int getID() const {return id;}
	inline void setID(int i) {id = i;}
	
private:
	
	QString name;
	Location* country;
	Location* province;
	int id;
};

/// Keeps a list of clubs
class ClubDB : public QAbstractTableModel
{
Q_OBJECT
public:
	
	~ClubDB();
	
	bool saveToFile(const QString path_prefix = "");
	bool loadFromFile();
	
	void addClub(Club* club, bool setNewID = true);
	void deleteClub(Club* club);
	void mergeClubs(Club* src, Club* dest);
	
	Club* getItem(const QModelIndex &index) const;
	bool containsName(const QString& name, Club* exclude = NULL);
	Club* findClub(const QString& name);
	Club* findClubAt(Location* location, Club* exclude = NULL);
	void findSimilarClubs(QString name, std::vector<Club*>& out);
	inline void clubChanged(QModelIndex index)
	{
		emit dataChanged(index, index);
	}
	
	inline int getNextID() const {return nextID;}
	void setNextID(int i) {nextID = i;}
	int getFreeID();
	Club* getByID(int id);
	
	// Overridden from QAbstractTableModel
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	
	virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	
	virtual bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
	virtual bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
	
	virtual Qt::ItemFlags flags(const QModelIndex& index) const;
	
	static inline ClubDB& getSingleton()
	{
		static ClubDB instance;
		return instance;
	}
	
private:
	
	typedef std::vector< Club* > Clubs;
	
	ClubDB();
	
	/// Appends #2, #3, ... in case the desiredName is already used until an unused name is found
	QString getUnusedName(const QString& desiredName);
	
	int nextID;
	std::map<int, Club*> id_map;
	std::map<int, int> merge_info;
	Clubs clubs;
};

#define clubDB ClubDB::getSingleton()

class ClubTable : public QTableView
{
Q_OBJECT
public:
	
	ClubTable();
	~ClubTable();
	
	inline QSortFilterProxyModel* getSortedModel() const {return sortedModel;}
	
public slots:
	
	void sort();
	
private:
	
	QSortFilterProxyModel* sortedModel;
};

#endif
