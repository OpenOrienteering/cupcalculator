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


#ifndef CUPCALCULATOR_RESULTLIST_H
#define CUPCALCULATOR_RESULTLIST_H

#include <vector>

#include <QAbstractTableModel>
#include <QTableView>
#include <QTreeView>
#include <QSortFilterProxyModel>

QT_BEGIN_NAMESPACE
class QXmlStreamWriter;
class QXmlStreamReader;
QT_END_NAMESPACE

class SeriesScoring;
class CSVFile;

/// Generic result list where custom columns can be added
class ResultList : public QAbstractTableModel
{
Q_OBJECT
public:
	
	/// Possible values for the ColumnResult column
	enum ResultType
	{
		// NOTE: the order defines the sorting order in result lists
		ResultOk = 0,
		ResultOvertime = 1,
		ResultMispunch = 2,
		ResultDidNotFinish = 3,
		ResultDisqualified = 4,
		ResultDidNotStart = 5
	};
	
	enum ColumnType
	{
		ColumnRank = 0,			// Normal integer; can be -1 if runner not ranked (mispunch, dnf, etc.),
									// -2 if runner not ranked and not in scoring (dnf etc. and not in scoring or noncompetitive) or
									// QVariant() if runner ranked, but not in scoring or noncompetitive
		ColumnRunner = 1,		// Pointer to runner
		ColumnClub = 6,			// Pointer to club
		ColumnPoints = 2,		// Integer which gets divided by decimal_factor for display; can be QVariant() if 0 points and "-" should be displayed
		ColumnPointInfo = 7,	// Like ColumnPoints, but is not taken into account for point calculation
		ColumnTime = 3,			// Integer: number of 1/100 seconds
		ColumnResult = 4,		// ResultType enum
		ColumnCategory = 5		// Pointer to category
	};
	
	/// Creates an empty result list
	ResultList(QString title, int decimal_places);
	/// Copies the layout (and content, if specified) of an existing result list
	ResultList(const ResultList& other, bool copy_content);
	/// Loads the list from the stream, takes the category pointers from the given SeriesScoring category list
	ResultList(QXmlStreamReader* stream, bool use_ids, SeriesScoring* scoring);
	~ResultList();
	
	void exportToCSV(CSVFile* file);
	void saveToFile(QXmlStreamWriter* stream);
	
	/// Adds a new column. If the result list contains rows, the new column is filled with default values.
	/// Specify -1 as position to append the column at the end. Returns the index of the new column
	int addColumn(ColumnType type, QString label = QString(), int pos = -1);
	
	/// Adds category, rank, runner, result, time
	void addRaceTimesColumns(int num_runners);
	
	/// Adds a new row. You can only add rows after at least one column has been added. Specify -1 as position to append the row at the end. Returns the new row's position
	int addRow(int new_row = -1);
	
	/// Looks for v in the given column. Returns the first row found or -1 if not found
	int findInColumn(int column, QVariant v);
	
	/// Changes all occurences of src in the given column to dest
	void changeInColumn(int column, QVariant src, QVariant dest);
	
	/// Recalculates the ranks and sorts the result list by rank. Pass -1 for statusColumn if there is no status column
	void calculateRanks(int baseColumn, int statusColumn, bool ascending, bool keepNegativeRankings = false);
	
	/// Checks if the layout of this result list matches that of the other one (that is, if the same column types are included)
	bool layoutMatches(ResultList* other);
	
	ColumnType getColumnType(int pos);
	QString getColumnLabel(int pos);
	inline void setColumnLabel(int column, const QString& label) {columnLabel[column] = label;}
	QVariant getData(int row, int column);
	void setData(int row, int column, QVariant value);
	
	inline int getCategoryColumn() const {return column_category;}
	inline int getRankColumn() const {return column_rank;}
	inline int getFirstRunnerColumn() const {return column_first_runner;}
	inline int getLastRunnerColumn() const {return column_last_runner;}
	inline int getClubColumn() const {return column_club;}
	inline int getStatusColumn() const {return column_status;}
	inline int getTimeColumn() const {return column_time;}
	inline void setTimeColumn(int c) {column_time = c;}
	inline int getPointsColumn() const {return column_points;}
	inline void setPointsColumn(int c) {column_points = c;}
	inline int getAdditionalStartColumn() const {return column_additional_start;}
	inline void setAdditionalStartColumn(int c) {column_additional_start = c;}
	inline int getAdditionalEndColumn() const {return column_additional_end;}
	inline void setAdditionalEndColumn(int c) {column_additional_end = c;}
	
	inline const QString& getTitle() const {return title;}
	inline void setTitle(const QString& new_title) {title = new_title;}
	inline int getDecimalPlaces() const {return decimal_places;}
	inline int getDecimalFactor() const {return decimal_factor;}
	
	QVariant* getItem(const QModelIndex &index) const;
	QString getItemString(ColumnType type, QVariant* item) const;
	
	// Overridden from QAbstractTableModel
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	
	virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	
	virtual bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
	virtual bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
	
	virtual bool insertColumns(int column, int count, const QModelIndex& parent = QModelIndex());
	virtual bool removeColumns(int column, int count, const QModelIndex& parent = QModelIndex());
	
	virtual Qt::ItemFlags flags(const QModelIndex& index) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	
private:
	
	bool columnMatches(int a, int b);
	
	/*
	Picture:
		/--------------------------------------
		| columnTypes[0] | columnTypes[1] | ...
		|--------------------------------------
		| columns[0][0]  | columns[1][0]  | ...
		|--------------------------------------
		| columns[0][1]  | columns[1][1]  | ...
		|
	*/
	
	typedef std::vector< ColumnType > ColumnTypes;
	typedef std::vector< QString > ColumnLabel;
	
	typedef std::vector< QVariant > Data;
	typedef std::vector< Data > Columns;
	
	QString defaultColumnLabelForType(ColumnType type);
	bool compareResults(int a, int b, int baseColumn, int statusColumn, bool ascending);
	bool compareResults(int a, QVariant* b, int baseColumn, int statusColumn, bool ascending);
	
	ColumnTypes columnTypes;
	ColumnLabel columnLabel;
	Columns columns;
	
	int column_category;
	int column_rank;
	int column_first_runner;
	int column_last_runner;
	int column_club;
	int column_status;
	int column_time;
	int column_points;
	int column_additional_start;
	int column_additional_end;
	
	int decimal_places;
	int decimal_factor;
	
	QString title;
};

class ResultsTable : public QTableView
{
Q_OBJECT
public:
	
	ResultsTable(ResultList* results, QWidget* parent = nullptr);
	~ResultsTable();
	
    virtual void setModel(QAbstractItemModel* model);
};

class ResultsTree : public QTreeView
{
Q_OBJECT
public:
	
	ResultsTree(ResultList* results, int groupByCol, int sortByCol, QWidget* parent = nullptr);
	~ResultsTree();
	
	virtual void setModel(QAbstractItemModel* model);
	
private:
	
	class GroupSortProxyModel : public QAbstractProxyModel
	{
	public:
		GroupSortProxyModel(int groupByCol, QObject* parent = nullptr);
        virtual ~GroupSortProxyModel();
		
        virtual void setSourceModel(QAbstractItemModel* sourceModel);
		
        virtual QModelIndex mapFromSource(const QModelIndex& sourceIndex) const;
        virtual QModelIndex mapToSource(const QModelIndex& proxyIndex) const;
		
        virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex& child) const;
        virtual QVariant data(const QModelIndex& proxyIndex, int role = Qt::DisplayRole) const;
        virtual Qt::ItemFlags flags(const QModelIndex& index) const;
		
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
        virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
		
	private:
		struct GroupItem
		{
			GroupItem(QVariant value);
			
			QVariant value;
			std::vector< QModelIndex > children;
		};
		struct QVariantCompare
		{
			bool operator() (QVariant l, QVariant r);
		};
		
		int groupByCol;
		typedef std::vector< GroupItem > GroupItems;
		GroupItems groupItems;
	};
	
	GroupSortProxyModel* groupSortModel;
};

#endif
