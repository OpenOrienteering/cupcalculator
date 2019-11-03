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


#ifndef CUPCALCULATOR_RUNNER_H
#define CUPCALCULATOR_RUNNER_H

#include <functional>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include <Qt>
#include <QtGlobal>

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QTableView>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QSortFilterProxyModel;
class QStringListModel;
class QXmlStreamWriter;
QT_END_NAMESPACE

class Club;
class ComboBoxDelegate;

/// Stores runner data
/// TODO: Chip list
class Runner : public QObject
{
Q_OBJECT
public:
	
	Runner();
	Runner(const Runner& other);
	~Runner();
	
	/// Schema: FirstName LastName (Year)
	QString getShortDesc();
	/// Schema: FirstName LastName (Gender, Year)
	QString getFullDesc();
	
	void saveToFile(QXmlStreamWriter* stream, bool save_club_list);
	
	inline void setFirstName(const QString& value) {first_name = value;}
	inline const QString& getFirstName() const {return first_name;}
	
	inline void setLastName(const QString& value) {last_name = value;}
	inline const QString& getLastName() const {return last_name;}
			
	inline void setYear(int _year) {year = _year;}
	inline int getYear() const {return year;}
	
	inline void setIsMale(int _male) {male = _male;}
	inline bool isMale() const {return male;}
	
	void addClub(Club* club);		// does nothing if runner is in club already
	bool removeClub(Club* club);	// does nothing if runner is not in club; returns true if runner was in club before
	bool isInClub(Club* club);
	inline int getNumClubs() {return (int)clubs.size();}
	inline Club* getClub(int i) {return clubs[i];}
	inline void clearClubs() {clubs.clear();}
	QString getClubListString(const QString& separator);
	
	inline int getID() const {return id;}
	inline void setID(int i) {id = i;}
	
	bool operator==(const Runner& other) const;
	
private:
	
	typedef std::vector< Club* > ClubList;
	
	QString first_name;
	QString last_name;
	int year;
	bool male;
	ClubList clubs;
	int id;
};

/// Keeps a list of runners
class RunnerDB : public QAbstractTableModel
{
Q_OBJECT
public:
	
	~RunnerDB();
	
	bool saveToFile(const QString path_prefix = "");
	bool loadFromFile();
	
	void addRunner(Runner* data, bool setNewID = true);
	void deleteRunner(Runner* runner);
	
	Runner* findRunner(const Runner& data);
	Runner* findRunner(const QString& first_name, const QString& last_name, int year, bool isMale);	// NOTE: clubs are not checked by this method!
	std::vector<Runner*> findRunnersInClub(Club* club);
	void findSimilarRunners(Runner* runner, std::set< Runner*, std::less< Runner* >, std::allocator< Runner* > >& out);
	Runner* getItem(const QModelIndex &index) const;
	
	void clubsChangedInRow(int row);
	void changeClubForAllRunners(Club* src, Club* dest);
	
	/// Replaces all ocurrences of src by dest
	void mergeRunners(Runner* src, Runner* dest);
	
	inline int getNextID() const {return nextID;}
	void setNextID(int i) {nextID = i;}
	int getFreeID();
	Runner* getByID(int id);
	
	// Overridden from QAbstractTableModel
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	
	virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	
	virtual bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
	virtual bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
	
	virtual Qt::ItemFlags flags(const QModelIndex& index) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	
	static inline RunnerDB& getSingleton()
	{
		static RunnerDB instance;
		return instance;
	}
	
private:
	
	typedef std::vector< Runner* > Runners;
	
	RunnerDB();
	void _findSimilarRunners(Runner* runner, std::set< Runner*, std::less< Runner* >, std::allocator< Runner* > >& out);
	
	std::vector< std::pair<QString, QString> > nicknames;
	
	int nextID;
	std::map<int, Runner*> id_map;
	std::map<int, int> merge_info;
	Runners runners;
};

#define runnerDB RunnerDB::getSingleton()

class RunnerTable : public QTableView
{
Q_OBJECT
public:
	
	RunnerTable();
	~RunnerTable();
	
	inline QSortFilterProxyModel* getSortedModel() const {return sortedModel;}
	
public slots:
	
	void sort();
	
private:
	
	QStringListModel* genderList;
	ComboBoxDelegate* genderDelegate;
	
	//QSortFilterProxyModel* sortedClubModel;
	//ComboBoxDelegate* clubDelegate;
	
	QSortFilterProxyModel* sortedModel;
};

#endif
