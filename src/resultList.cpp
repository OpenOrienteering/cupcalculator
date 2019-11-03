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


#include "resultList.h"

#include "assert.h"
#include <math.h>

#include <QHeaderView>
#include <QDate>
#include <QDateTime>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QMessageBox>

#include "util.h"
#include "event.h"
#include "runner.h"
#include "csvFile.h"
#include "club.h"
#include "seriesScoring.h"

ResultList::ResultList(QString title, int decimal_places)
{
	this->title = title;
	this->decimal_places = decimal_places;
	decimal_factor = static_cast<int>(pow(10.0, decimal_places) + 0.5);
	
	column_category = -1;
	column_rank = -1;
	column_first_runner = -1;
	column_last_runner = -1;
	column_club = -1;
	column_status = -1;
	column_time = -1;
	column_points = -1;
	column_additional_start = -1;
	column_additional_end = -1;
}
ResultList::ResultList(const ResultList& other, bool copy_content)
{
	title = other.title;
	decimal_places = other.decimal_places;
	decimal_factor = other.decimal_factor;
	
	column_category = other.column_category;
	column_rank = other.column_rank;
	column_first_runner = other.column_first_runner;
	column_last_runner = other.column_last_runner;
	column_club = other.column_club;
	column_status = other.column_status;
	column_time = other.column_time;
	column_points = other.column_points;
	column_additional_start = other.column_additional_start;
	column_additional_end = other.column_additional_end;
	
	columnTypes = other.columnTypes;
	columnLabel = other.columnLabel;
	
	if (copy_content)
		columns = other.columns;
	else
	{
		std::vector< QVariant > new_vector;
		int num_columns = columnTypes.size();
		for (int i = 0; i < num_columns; ++i)
			columns.push_back(new_vector);
	}
}
ResultList::ResultList(QXmlStreamReader* stream, bool use_ids, SeriesScoring* scoring)
{
	title = stream->attributes().value("title").toString();
	decimal_places = stream->attributes().value("decimal_places").toString().toInt();
	decimal_factor = stream->attributes().value("decimal_factor").toString().toInt();
	
	column_category = stream->attributes().value("column_category").toString().toInt();
	column_rank = stream->attributes().value("column_rank").toString().toInt();
	column_first_runner = stream->attributes().value("column_first_runner").toString().toInt();
	column_last_runner = stream->attributes().value("column_last_runner").toString().toInt();
	column_club = stream->attributes().value("column_club").toString().toInt();
	column_status = stream->attributes().value("column_status").toString().toInt();
	column_time = stream->attributes().value("column_time").toString().toInt();
	column_points = stream->attributes().value("column_points").toString().toInt();
	column_additional_start = stream->attributes().value("column_additional_start").toString().toInt();
	column_additional_end = stream->attributes().value("column_additional_end").toString().toInt();
	
	int num_rows = stream->attributes().value("num_rows").toString().toInt();
	int num_columns = stream->attributes().value("num_cols").toString().toInt();
	
	columnTypes.reserve(num_columns);
	columnLabel.reserve(num_columns);
	columns.resize(num_columns);
	for (int i = 0; i < num_columns; ++i)
		columns[i].resize(num_rows);
	
	std::vector<int> rowsToDelete;
	int cur_row = -1;
	int cur_column = -1;
	while (!stream->atEnd())
	{
		stream->readNext();
		if (stream->tokenType() != QXmlStreamReader::StartElement)
		{
			if (stream->tokenType() == QXmlStreamReader::EndElement && stream->name() == "ResultList")
				return;
			continue;
		}
		
		if (stream->name() == "Value")
		{
			++cur_column;
			
			ColumnType type = columnTypes[cur_column];
			QString temp;
			switch (type)
			{
				case ColumnRank: case ColumnTime: case ColumnResult:
					columns[cur_column][cur_row] = stream->attributes().value("v").toString().toInt();
					break;
				case ColumnCategory:
					temp = stream->attributes().value("v").toString();
					columns[cur_column][cur_row] = qVariantFromValue<void*>(scoring->findInternCategory(temp));
					break;
				case ColumnRunner:
				{
					if (use_ids)
					{
						Runner* runner = runnerDB.getByID(stream->attributes().value("i").toString().toInt());
						if (runner)
							columns[cur_column][cur_row] = qVariantFromValue<void*>(runner);
						else
							rowsToDelete.push_back(cur_row);
					}
					else
					{
						QString first_name = stream->attributes().value("f").toString();
						QString last_name = stream->attributes().value("l").toString();
						int year = stream->attributes().value("y").toString().toInt();
						bool isMale = stream->attributes().value("m") == "yes";
						
						Runner* runner = runnerDB.findRunner(first_name, last_name, year, isMale);
						if (!runner)
						{
							QMessageBox::warning(nullptr, tr("Error"), tr("Error while loading series scoring: cannot find runner %1 %2 in runner database!").arg(first_name, last_name));
							return;
						}
						columns[cur_column][cur_row] = qVariantFromValue<void*>(runner);
					}
					break;
				}
				case ColumnClub:
				{
					if (use_ids)
					{
						temp = stream->attributes().value("v").toString();
						if (temp == "")
							columns[cur_column][cur_row] = qVariantFromValue<void*>(nullptr);
						else
						{
							Club* club = clubDB.getByID(stream->attributes().value("i").toString().toInt());
							if (club)
								columns[cur_column][cur_row] = qVariantFromValue<void*>(club);
							else
								rowsToDelete.push_back(cur_row);
						}
					}
					else
					{
						temp = stream->attributes().value("v").toString();
						if (temp == "")
							columns[cur_column][cur_row] = qVariantFromValue<void*>(nullptr);
						else
						{
							Club* club = clubDB.findClub(temp);
							if (!club)
							{
								QMessageBox::warning(nullptr, tr("Error"), tr("Error while loading series scoring: cannot find club %1 in runner database!").arg(temp));
								return;
							}
							columns[cur_column][cur_row] = qVariantFromValue<void*>(club);
						}
					}
					break;
				}
				case ColumnPoints: case ColumnPointInfo:
					temp = stream->attributes().value("v").toString();
					if (temp == "-")
						columns[cur_column][cur_row] = QVariant();
					else
						columns[cur_column][cur_row] = temp.toInt();
					break;
			}
		}
		else if (stream->name() == "Row")
		{
			++cur_row;
			cur_column = -1;
		}
		else if (stream->name() == "Column")
		{
			ColumnType type = static_cast<ColumnType>(stream->attributes().value("type").toString().toInt());
			columnTypes.push_back(type);
			
			QString label = stream->attributes().value("label").toString();
			columnLabel.push_back(label);
		}
	}
	
	for (int i = (int)rowsToDelete.size() - 1; i >= 0; --i)
		removeRow(rowsToDelete[i]);
}
ResultList::~ResultList()
{
}

void ResultList::saveToFile(QXmlStreamWriter* stream)
{
	int num_rows = rowCount();
	int num_columns = columnCount();
	
	stream->writeStartElement("ResultList");
	stream->writeAttribute("title", title);
	stream->writeAttribute("decimal_places", QString::number(decimal_places));
	stream->writeAttribute("decimal_factor", QString::number(decimal_factor));
	
	stream->writeAttribute("column_category", QString::number(column_category));
	stream->writeAttribute("column_rank", QString::number(column_rank));
	stream->writeAttribute("column_first_runner", QString::number(column_first_runner));
	stream->writeAttribute("column_last_runner", QString::number(column_last_runner));
	stream->writeAttribute("column_club", QString::number(column_club));
	stream->writeAttribute("column_status", QString::number(column_status));
	stream->writeAttribute("column_time", QString::number(column_time));
	stream->writeAttribute("column_points", QString::number(column_points));
	stream->writeAttribute("column_additional_start", QString::number(column_additional_start));
	stream->writeAttribute("column_additional_end", QString::number(column_additional_end));
	
	stream->writeAttribute("num_rows", QString::number(num_rows));
	stream->writeAttribute("num_cols", QString::number(num_columns));
	

	for (int i = 0; i < num_columns; ++i)
	{
		stream->writeEmptyElement("Column");
		stream->writeAttribute("type", QString::number(static_cast<int>(columnTypes[i])));
		stream->writeAttribute("label", columnLabel[i]);
	}

	for (int i = 0; i < num_rows; ++i)
	{
		stream->writeStartElement("Row");
		
		for (int k = 0; k < num_columns; ++k)
		{
			stream->writeEmptyElement("Value");

			ColumnType type = columnTypes[k];
			switch (type)
			{
				case ColumnRank: case ColumnTime: case ColumnResult:
					stream->writeAttribute("v", QString::number(columns[k][i].toInt()));
					break;
				case ColumnCategory:
					stream->writeAttribute("v", reinterpret_cast<AbstractCategory*>(columns[k][i].value<void*>())->name);
					break;
				case ColumnRunner:
				{
					Runner* runner = reinterpret_cast<Runner*>(columns[k][i].value<void*>());
					stream->writeAttribute("i", QString::number(runner->getID()));
					stream->writeAttribute("f", runner->getFirstName());
					stream->writeAttribute("l", runner->getLastName());
					stream->writeAttribute("m", runner->isMale() ? "yes" : "no");
					stream->writeAttribute("y", QString::number(runner->getYear()));
					break;
				}
				case ColumnClub:
				{
					Club* club = reinterpret_cast<Club*>(columns[k][i].value<void*>());
					stream->writeAttribute("v", club ? club->getName() : "");
					stream->writeAttribute("i", club ? QString::number(club->getID()) : "-1");
					break;
				}
				case ColumnPoints: case ColumnPointInfo:
					stream->writeAttribute("v", columns[k][i].isValid() ? QString::number(columns[k][i].toInt()) : "-");
					break;
			}
		}
		
		stream->writeEndElement();
	}
	
	stream->writeEndElement();
}

void ResultList::addRaceTimesColumns(int num_runners)
{
	assert(num_runners < 1000);
	addColumn(ColumnCategory);
	addColumn(ColumnRank);
	if (num_runners == 1)
		addColumn(ColumnRunner);
	else
	{
		for (int i = 0; i < num_runners; ++i)
		{
			addColumn(ColumnRunner);
			addColumn(ColumnTime, tr("Runner time"));
		}
	}
	addColumn(ColumnClub);
	
	addColumn(ColumnResult);
	int col_time = addColumn(ColumnTime);
	setTimeColumn(col_time);
}

QString ResultList::defaultColumnLabelForType(ResultList::ColumnType type)
{
	switch (type)
	{
	case ColumnRank:		return tr("Rank");
	case ColumnRunner:		return tr("Runner");
	case ColumnClub:		return tr("Club");
	case ColumnPointInfo:
	case ColumnPoints:		return tr("Points");
	case ColumnTime:		return tr("Time");
	case ColumnResult:		return tr("Result");
	case ColumnCategory:	return tr("Category");
	default:				return tr("Unknown");
	}
}

void ResultList::calculateRanks(int baseColumn, int statusColumn, bool ascending, bool keepNegativeRankings)
{
	// Step 1: Heapsort from Wikipedia
	const int WIDTH = 8;				// number of children of nodes
	int parent, child, w, max, i;	// , val
	int num_columns = columns.size();
	QVariant* val_data = new QVariant[num_columns];
	int n = columns[0].size();
	int m = (n + (WIDTH - 2)) / WIDTH;	// first leaf in tree
	
	if (n == 0)
		return;	// no rows
 
	while (true)
	{
		if (m)
		{
			// Part 1: construct heap
			parent = --m;
			//val = columns[baseColumn][parent].toInt();
			for (int i = 0; i < num_columns; ++i)
				val_data[i] = columns[i][parent];
		}
		else if (--n )
		{
			// Part 2: sort
			//val = columns[baseColumn][n].toInt();
			for (int i = 0; i < num_columns; ++i)
				val_data[i] = columns[i][n];
			
			for (int i = 0; i < num_columns; ++i)
				columns[i][n] = columns[i][0];
			parent = 0;
		}
		else
		  break;

		while ((child = parent * WIDTH + 1) < n)
		{
			w = n - child < WIDTH ? n - child : WIDTH;

			for (max= 0, i= 1; i < w; ++i)
				if (compareResults(child+i, child+max, baseColumn, statusColumn, ascending))
					max = i;

			child += max;

			if (!compareResults(child, val_data, baseColumn, statusColumn, ascending))
				break;

			for (int i = 0; i < num_columns; ++i)
				columns[i][parent] = columns[i][child];
			parent = child;
		}

		for (int i = 0; i < num_columns; ++i)
			columns[i][parent] = val_data[i];
	}
	delete[] val_data;

	// Step 2: assign ranks
	int num_rows = rowCount();
	if (column_rank >= 0)
	{
		int cur_rank = 1;
		AbstractCategory* cur_cat = nullptr;
		int cur_value = -1;
		int rank_step = 1;
		for (int i = 0; i < num_rows; ++i)
		{
			if (columns[column_rank][i] == QVariant())	// If row is marked as noncompetitive, do not overwrite it
				continue;
			else if (keepNegativeRankings && columns[column_rank][i].toInt() < 0)
				continue;
			
			AbstractCategory* cat = (column_category >= 0) ? reinterpret_cast<AbstractCategory*>(columns[column_category][i].value<void*>()) : nullptr;
			if (cat != cur_cat)
			{
				cur_rank = 1;
				cur_cat = cat;
				rank_step = 1;
			}
			else if (columns[baseColumn][i].toInt() == cur_value)
			{
				cur_rank -= rank_step;
				++rank_step;
			}
			else
				rank_step = 1;
			
			if (statusColumn >= 0 && columns[statusColumn][i] != ResultOk)
				columns[column_rank][i] = -1;
			else
				columns[column_rank][i] = cur_rank;
			cur_rank += rank_step;
			cur_value = columns[baseColumn][i].toInt();
		}
	}
	
	emit dataChanged(index(0, 0), index(num_columns - 1, num_rows - 1));
}
bool ResultList::compareResults(int a, int b, int baseColumn, int statusColumn, bool ascending)
{
	if (column_category >= 0)
	{
		AbstractCategory* cat_a = reinterpret_cast<AbstractCategory*>(columns[column_category][a].value<void*>());
		AbstractCategory* cat_b = reinterpret_cast<AbstractCategory*>(columns[column_category][b].value<void*>());
		
		if (cat_a->number != cat_b->number)
			return cat_a->number > cat_b->number;
	}
	
	if (statusColumn >= 0 && columns[statusColumn][a].toInt() != columns[statusColumn][b].toInt())
		return columns[statusColumn][a].toInt() > columns[statusColumn][b].toInt();
	else if (ascending)
		return columns[baseColumn][a].toInt() > columns[baseColumn][b].toInt();
	else
		return columns[baseColumn][a].toInt() < columns[baseColumn][b].toInt();
}
bool ResultList::compareResults(int a, QVariant* b, int baseColumn, int statusColumn, bool ascending)
{
	if (column_category >= 0)
	{
		AbstractCategory* cat_a = reinterpret_cast<AbstractCategory*>(columns[column_category][a].value<void*>());
		AbstractCategory* cat_b = reinterpret_cast<AbstractCategory*>(b[column_category].value<void*>());
		
		if (cat_a->number != cat_b->number)
			return cat_a->number > cat_b->number;
	}
	
	if (statusColumn >= 0 && columns[statusColumn][a].toInt() != b[statusColumn].toInt())
		return columns[statusColumn][a].toInt() > b[statusColumn].toInt();
	else if (ascending)
		return columns[baseColumn][a].toInt() > b[baseColumn].toInt();
	else
		return columns[baseColumn][a].toInt() < b[baseColumn].toInt();
}

bool ResultList::layoutMatches(ResultList* other)
{
	return columnMatches(column_category, other->column_category) &&
			columnMatches(column_rank, other->column_rank) &&
			columnMatches(column_first_runner, other->column_first_runner) &&
			columnMatches(column_last_runner, other->column_last_runner) &&
			columnMatches(column_club, other->column_club) &&
			columnMatches(column_status, other->column_status) &&
			columnMatches(column_time, other->column_time) &&
			columnMatches(column_points, other->column_points);
}
bool ResultList::columnMatches(int a, int b)
{
	return (a == b || (a >= 0 && b >= 0));
}

int ResultList::findInColumn(int column, QVariant v)
{
	int numRows = rowCount();
	for (int i = 0; i < numRows; ++i)
	{
		if (columns[column][i] == v)
			return i;
	}
	return -1;
}

void ResultList::changeInColumn(int column, QVariant src, QVariant dest)
{
	int numRows = rowCount();
	for (int i = 0; i < numRows; ++i)
	{
		if (columns[column][i] == src)
			setData(i, column, dest);
	}
}

int ResultList::addRow(int new_row)
{
	size_t num_columns = columnCount();
	size_t num_rows = rowCount();
	if (new_row < 0)
		new_row = num_rows;
	
	for (size_t i = 0; i < num_columns; ++i)
		columns[i].insert(columns[i].begin() + new_row, QVariant());
	
	return new_row;
}
int ResultList::addColumn(ResultList::ColumnType type, QString label, int pos)
{
	size_t num_columns = columnCount();
	size_t num_rows = rowCount();
	if (pos < 0)
		pos = num_columns;
	
	std::vector< QVariant > new_vector;
	columns.insert(columns.begin() + pos, new_vector);
	columnTypes.insert(columnTypes.begin() + pos, type);
	columnLabel.insert(columnLabel.begin() + pos, label.isEmpty() ? defaultColumnLabelForType(type) : label);
	
	if (num_rows > 0)
	{
		columns[pos].reserve(num_rows);
		for (size_t i = 0; i < num_rows; ++i)
			columns[pos].push_back(QVariant());
	}
	
	if (type == ColumnCategory && column_category < 0)
		column_category = pos;
	else if (type == ColumnRank && column_rank < 0)
		column_rank = pos;
	else if (type == ColumnRunner)
	{
		if (column_first_runner < 0 || column_first_runner > pos)
			column_first_runner = pos;
		if (column_last_runner < 0 || column_last_runner < pos)
			column_last_runner = pos;
	}
	else if (type == ColumnClub && column_club < 0)
		column_club = pos;
	else if (type == ColumnResult && column_status < 0)
		column_status = pos;
	else if (type == ColumnTime && column_time < 0)
		column_time = pos;
	else if (type == ColumnPoints && column_points < 0)
		column_points = pos;
	
	return pos;
}
ResultList::ColumnType ResultList::getColumnType(int pos)
{
	return columnTypes[pos];
}
QString ResultList::getColumnLabel(int pos)
{
	return columnLabel[pos];
}

QVariant ResultList::getData(int row, int column)
{
	return columns[column][row];
}
void ResultList::setData(int row, int column, QVariant value)
{
	setData(index(row, column), value);
}

// QAbstractTableModel stuff

QVariant* ResultList::getItem(const QModelIndex& index) const
{
	if (index.isValid())
	{
		QVariant* item = static_cast<QVariant*>(index.internalPointer());
		if (item)
			return item;
	}
	
	return nullptr;
}

int ResultList::rowCount(const QModelIndex& parent) const
{
	if (columns.empty())
		return 0;
	
	return columns[0].size();
}
int ResultList::columnCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;	// the docs say that when implementing a table based model, columnCount() should return 0 when the parent is valid
	
	return columns.size();
}

QModelIndex ResultList::index(int row, int column, const QModelIndex& parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();
	if (row < 0 || row >= rowCount())
		return QModelIndex();
	if (column < 0 || column >= columnCount())
		return QModelIndex();
	
	return createIndex(row, column, const_cast<QVariant*>(&columns[column][row]));
}

QVariant ResultList::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= 0 && section < (int)columnLabel.size())
		return QVariant(columnLabel[section]);
		
	return QVariant();
}

Qt::ItemFlags ResultList::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return {};
	
	// TODO!
	return /*Qt::ItemIsEditable |*/ Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ResultList::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role != Qt::DisplayRole && role != Qt::EditRole)
		return QVariant();
	
	QVariant* item = getItem(index);
	if (!item)
		return QVariant();
	
	ColumnType type = columnTypes[index.column()];
	return getItemString(type, item);
}
QString ResultList::getItemString(ResultList::ColumnType type, QVariant* item) const
{	
	const QString resultTypeTexts[6] = {
		tr("OK"),
		tr("OT"),
		tr("MP"),
		tr("DNF"),
		tr("DISQ"),
		tr("DNS")
	};
	
	switch (type)
	{
	case ColumnRank:
		if (item->isValid() && item->toInt() > 0)
			return item->toString();
		else
			return "-";
	case ColumnCategory:
		return reinterpret_cast<AbstractCategory*>(item->value<void*>())->name;
	case ColumnRunner:
	{
		Runner* runner = reinterpret_cast<Runner*>(item->value<void*>());
		if (runner)
			return runner->getShortDesc();
		else
			return "";
	}
	case ColumnClub:
	{
		Club* club = reinterpret_cast<Club*>(item->value<void*>());
		if (club)
			return club->getName();
		else if (item->type() == QVariant::String)
			return item->toString();
		else
			return tr("- no club -");
	}
	case ColumnPoints: case ColumnPointInfo:
		return item->isValid() ? pointsToString(item->toInt(), decimal_places, decimal_factor) : "-";
	case ColumnTime:
		return item->isValid() ? timeToString(item->toInt()) : "";
	case ColumnResult:
		return resultTypeTexts[item->toInt()];
	default:
		return item->toString();
	}
}
bool ResultList::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (role != Qt::EditRole)
		return false;
	
	QVariant* item = getItem(index);
	if (!item)
		return false;
	
	*item = value;
	
	emit dataChanged(index, index);
	return true;
}

bool ResultList::insertColumns(int column, int count, const QModelIndex& parent)
{
	// TODO
	return QAbstractItemModel::insertColumns(column, count, parent);
}
bool ResultList::removeColumns(int column, int count, const QModelIndex& parent)
{
	if (column < 0 || column + count > static_cast<int>(columns.size()))
		return false;
	
	beginRemoveColumns(parent, column, column + count - 1);
	columns.erase(columns.begin() + column, columns.begin() + (column + count));
	columnTypes.erase(columnTypes.begin() + column, columnTypes.begin() + (column + count));
	columnLabel.erase(columnLabel.begin() + column, columnLabel.begin() + (column + count));
	
	for (int i = column; i < column + count; ++i)
	{
		if (i == column_category)				column_category = -1;
		else if (i == column_rank)				column_rank = -1;
		else if (i == column_first_runner)		column_first_runner = -1;
		else if (i == column_last_runner)		column_last_runner = -1;
		else if (i == column_club)				column_club = -1;
		else if (i == column_status)			column_status = -1;
		else if (i == column_time)				column_time = -1;
		else if (i == column_points)			column_points = -1;
		else if (i == column_additional_start)	column_additional_start = -1;
		else if (i == column_additional_end)	column_additional_end = -1;
	}
	endRemoveColumns();
	
	return true;
}

bool ResultList::insertRows(int row, int count, const QModelIndex& parent)
{
	// TODO
	return QAbstractItemModel::insertRows(row, count, parent);
}
bool ResultList::removeRows(int row, int count, const QModelIndex& parent)
{
	if (row < 0 || row + count > rowCount(parent))
		return false;
	
	beginRemoveRows(parent, row, row + count - 1);
	for (int c = 0; c < columnCount(parent); ++c)
		columns[c].erase(columns[c].begin() + row, columns[c].begin() + (row + count - 1));
	endRemoveRows();
	
	return true;
}

void ResultList::exportToCSV(CSVFile* file)
{
	// Column names
	QStringList stringList;
	int numColumns = columnCount();
	for (int i = 0; i < numColumns; ++i)
	{
		ColumnType type = columnTypes[i];
		if (type == ColumnRunner)
			stringList << tr("First name") << tr("Surname");
		else
			stringList << columnLabel[i];
	}
	
	file->writeLine(stringList);
	stringList.clear();
	
	int numRows = rowCount();
	for (int row = 0; row < numRows; ++row)
	{
		for (int i = 0; i < numColumns; ++i)
		{
			ColumnType type = columnTypes[i];
			
			if (type == ColumnRunner)
			{
				Runner* runner = reinterpret_cast<Runner*>(getData(row, i).value<void*>());
				if (runner)
					stringList << runner->getFirstName() << runner->getLastName();
				else
					stringList << "" << "";
			}
			else
				stringList << data(index(row, i), Qt::DisplayRole).toString();
		}
		
		file->writeLine(stringList);
		stringList.clear();
	}
}

// ### ResultsTable ###

ResultsTable::ResultsTable(ResultList* results, QWidget* parent) : QTableView(parent)
{
	//sortedModel = new QSortFilterProxyModel();
	//sortedModel->setSourceModel(&runnerDB);
	setModel(results);
	//setSortingEnabled(true);
	//sortByColumn(0);
	setHorizontalScrollMode(ScrollPerPixel);
	
	verticalHeader()->setVisible(false);
}
ResultsTable::~ResultsTable()
{
	setModel(nullptr);
}
void ResultsTable::setModel(QAbstractItemModel* model)
{
    QTableView::setModel(model);
	
	for (int i = 0; i < horizontalHeader()->count(); ++i)
		horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
}

// ### ResultsTree ###

ResultsTree::ResultsTree(ResultList* results, int groupByCol, int sortByCol, QWidget* parent) : QTreeView(parent)
{
	//header()->hide();
	
	groupSortModel = new GroupSortProxyModel(groupByCol);
	groupSortModel->setSourceModel(results);
	QTreeView::setModel(groupSortModel);
	
	if (sortByCol >= 0)
		sortByColumn(sortByCol);
}
ResultsTree::~ResultsTree()
{
	QTreeView::setModel(nullptr);
	delete groupSortModel;
}
void ResultsTree::setModel(QAbstractItemModel* model)
{
	QTreeView::setModel(nullptr);
	groupSortModel->setSourceModel(model);
	QTreeView::setModel(groupSortModel);
}

ResultsTree::GroupSortProxyModel::GroupSortProxyModel(int groupByCol, QObject* parent): QAbstractProxyModel(parent)
{
	this->groupByCol = groupByCol;
}
ResultsTree::GroupSortProxyModel::~GroupSortProxyModel()
{

}

void ResultsTree::GroupSortProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    QAbstractProxyModel::setSourceModel(sourceModel);
	
	groupItems.clear();
	
	if (sourceModel)
	{
		// Create group items
		typedef std::map< QVariant, int, QVariantCompare > GroupMap;
		GroupMap groupMap;
		int numRows = sourceModel->rowCount();
		for (int i = 0; i < numRows; ++i)
		{
			QModelIndex ind = sourceModel->index(i, groupByCol, QModelIndex());
			QVariant v = sourceModel->data(ind, Qt::DisplayRole);
			
			GroupMap::iterator it = groupMap.find(v);
			if (it == groupMap.end())
			{
				it = groupMap.insert(GroupMap::value_type(v, groupItems.size())).first;
				groupItems.push_back(GroupItem(v));
			}
			
			groupItems[it->second].children.push_back(ind);
		}
	}
}

QModelIndex ResultsTree::GroupSortProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
	return QModelIndex();	// NOTE: unfinished, continue here ...
	
	
	//return index(sourceIndex.row(), sourceIndex.column(), sourceIndex.parent());
}
QModelIndex ResultsTree::GroupSortProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
	if (proxyIndex.parent() == QModelIndex())
		return QModelIndex();
	else
	{
		GroupItem* groupItem = static_cast<GroupItem*>(proxyIndex.parent().internalPointer());
		return groupItem->children[proxyIndex.row()];
		//return sourceModel()->index(proxyIndex.row(), proxyIndex.column(), QModelIndex());
	}
}

QModelIndex ResultsTree::GroupSortProxyModel::index(int row, int column, const QModelIndex& parent) const
{
	if (parent != QModelIndex())
		return QModelIndex();
	
	return createIndex(row, column, (void*)(&groupItems[row]));
}
QModelIndex ResultsTree::GroupSortProxyModel::parent(const QModelIndex& child) const
{
	return QModelIndex();
}
QVariant ResultsTree::GroupSortProxyModel::data(const QModelIndex& proxyIndex, int role) const
{
	if (proxyIndex.parent() == QModelIndex())
	{
		if (!proxyIndex.isValid())
			return QVariant();
		if (role != Qt::DisplayRole)
			return QVariant();
		
		GroupItem* groupItem = static_cast<GroupItem*>(proxyIndex.internalPointer());
		return groupItem->value;
	}
	else
		return QAbstractProxyModel::data(proxyIndex, role);
}
Qt::ItemFlags ResultsTree::GroupSortProxyModel::flags(const QModelIndex& index) const
{
	if (index.parent() == QModelIndex())
	{
		if (!index.isValid())
			return {};
		
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}
	else
		return QAbstractProxyModel::flags(index);
}

QVariant ResultsTree::GroupSortProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		if (sourceModel())
			return sourceModel()->headerData((section >= groupByCol) ? (section + 1) : section, orientation, role);
		else
			return QVariant();
	}
	else
		return QAbstractProxyModel::headerData(section, orientation, role);
}
int ResultsTree::GroupSortProxyModel::rowCount(const QModelIndex& parent) const
{
	if (parent == QModelIndex())
		return groupItems.size();
	else
	{
		GroupItem* groupItem = static_cast<GroupItem*>(parent.internalPointer());
		return groupItem->children.size();
	}
}
int ResultsTree::GroupSortProxyModel::columnCount(const QModelIndex& parent) const
{
	if (parent == QModelIndex())
		return sourceModel() ? 1 : 0;
	else
		return sourceModel()->columnCount() - 1;
}

ResultsTree::GroupSortProxyModel::GroupItem::GroupItem(QVariant value)
{
	this->value = value;
}
bool ResultsTree::GroupSortProxyModel::QVariantCompare::operator() (QVariant l, QVariant r)
{
	// CODE COPIED FROM QSortFilterProxyModel::lessThan
	
	switch (l.userType()) {
		case QVariant::Invalid:
			return (r.type() != QVariant::Invalid);
		case QVariant::Int:
			return l.toInt() < r.toInt();
		case QVariant::UInt:
			return l.toUInt() < r.toUInt();
		case QVariant::LongLong:
			return l.toLongLong() < r.toLongLong();
		case QVariant::ULongLong:
			return l.toULongLong() < r.toULongLong();
		case QMetaType::Float:
			return l.toFloat() < r.toFloat();
		case QVariant::Double:
			return l.toDouble() < r.toDouble();
		case QVariant::Char:
			return l.toChar() < r.toChar();
		case QVariant::Date:
			return l.toDate() < r.toDate();
		case QVariant::Time:
			return l.toTime() < r.toTime();
		case QVariant::DateTime:
			return l.toDateTime() < r.toDateTime();
		case QVariant::String:
		default:
			//if (d->sort_localeaware)
				return l.toString().localeAwareCompare(r.toString()) < 0;
			/*else
				return l.toString().compare(r.toString(), d->sort_casesensitivity) < 0;*/
	}
	return false;
}
