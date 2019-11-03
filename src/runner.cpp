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


#include "runner.h"

#include "assert.h"

#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QFile>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QMessageBox>

#include "club.h"
#include "comboBoxDelegate.h"
#include "util.h"
#include "config.h"
#include "seriesScoring.h"
#include "csvFile.h"

const int COLUMN_COUNT = 5;

// ### Runner ###

Runner::Runner() : first_name(""), last_name(""), year(0), male(false)
{
}
Runner::Runner(const Runner& other)
{
	first_name = other.first_name;
	last_name = other.last_name;
	year = other.year;
	male = other.male;
	clubs = other.clubs;
}
Runner::~Runner()
{
}

QString Runner::getShortDesc()
{
	QString str = getFirstName();
	if (!str.isEmpty())
		str += " ";
	str += getLastName();
	
	if (getYear() > 0)
		str += " (" + QString::number(getYear()) + ")";
	else
		str += " (?)";
	
	return str;
}
QString Runner::getFullDesc()
{
	QString str = getFirstName();
	if (!str.isEmpty())
		str += " ";
	str += getLastName();
	
	str += " (" + (isMale() ? tr("M") : tr("W")) + ((getYear() > 0) ? QString::number(getYear()) : "?") + ")";
	
	return str;
}

void Runner::saveToFile(QXmlStreamWriter* stream, bool save_club_list)
{
	int num_clubs = getNumClubs();
	
	if (num_clubs > 1 && save_club_list)
		stream->writeStartElement("Runner");
	else
		stream->writeEmptyElement("Runner");
	stream->writeAttribute("id", QString::number(id));
	stream->writeAttribute("first_name", getFirstName());
	stream->writeAttribute("last_name", getLastName());
	stream->writeAttribute("year", QString::number(getYear()));
	stream->writeAttribute("male", isMale() ? "yes" : "no");
	
	if (save_club_list)
	{
		if (num_clubs == 1)
			stream->writeAttribute("club", getClub(0)->getName());
		else if (num_clubs > 1)
		{
			for (int k = 0; k < num_clubs; ++k)
			{
				stream->writeEmptyElement("Club");
				stream->writeAttribute("name", getClub(k)->getName());
			}
			
			stream->writeEndElement();
		}
	}
}

void Runner::addClub(Club* club)
{
	if (isInClub(club))
		return;
	
	clubs.push_back(club);
}
bool Runner::removeClub(Club* club)
{
	for (size_t i = 0; i < clubs.size(); ++i)
	{
		if (clubs[i] == club)
		{
			clubs.erase(clubs.begin() + i);
			return true;
		}
	}
	return false;
}
bool Runner::isInClub(Club* club)
{
	for (size_t i = 0; i < clubs.size(); ++i)
	{
		if (clubs[i] == club)
			return true;
	}
	return false;
}
QString Runner::getClubListString(const QString& separator)
{
	QString str = (getNumClubs() > 0) ? getClub(0)->getName() : "";
	for (int i = 1; i < getNumClubs(); ++i)
		str += separator + getClub(i)->getName();
	return str;
}

bool Runner::operator==(const Runner & other) const
{
	return ((first_name == other.first_name) && (last_name == other.last_name) && (year == other.year) && (male == other.male));
}

// ### RunnerDB ###

RunnerDB::RunnerDB()
{
	nextID = 0;
}
RunnerDB::~RunnerDB()
{
	size_t size = runners.size();
	for (size_t i = 0; i < size; ++i)
		delete runners[i];
}

bool RunnerDB::saveToFile(const QString path_prefix)
{
	QString path = path_prefix + "databases/runners.xml";
	QFile file(path + "_new");
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	
	QXmlStreamWriter stream(&file);
	stream.setAutoFormatting(true);
	stream.writeStartDocument();
	stream.writeStartElement("Runners");
	stream.writeAttribute("installationKey", installationKey);
	stream.writeAttribute("fileFormatVersion", "1");
	
	for (std::map<int, int>::iterator it = merge_info.begin(); it != merge_info.end(); ++it)
	{
		stream.writeEmptyElement("Merge");
		stream.writeAttribute("from", QString::number(it->first));
		stream.writeAttribute("to", QString::number(it->second));
	}
	
	int size = runners.size();
	for (int i = 0; i < size; ++i)
	{
		Runner* runner = runners[i];
		runner->saveToFile(&stream, true);
	}
	
	stream.writeEndElement();
	stream.writeEndDocument();
	
	QFile::remove(path);
	file.rename(path);
	
	return true;
}
bool RunnerDB::loadFromFile()
{
	QFile file("databases/runners.xml");
	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		Runner* current_runner = nullptr;
	
		QXmlStreamReader stream(&file);
		while (!stream.atEnd())
		{
			stream.readNext();
			if (stream.tokenType() != QXmlStreamReader::StartElement)
				continue;
		
			if (stream.name() == "Runner")
			{
				if (current_runner)
					addRunner(current_runner, false);
			
				current_runner = new Runner();
				current_runner->setID(stream.attributes().value("id").toString().toInt());
				current_runner->setFirstName(stream.attributes().value("first_name").toString());
				current_runner->setLastName(stream.attributes().value("last_name").toString());
				current_runner->setYear(stream.attributes().value("year").toString().toInt());
				current_runner->setIsMale(stream.attributes().value("male").toString() == "yes");
			
				QStringRef ref = stream.attributes().value("club");
				if (!ref.isEmpty())
				{
					Club* club = clubDB.findClub(ref.toString());
					if (club)
						current_runner->addClub(club);
					else
						QMessageBox::warning(nullptr, tr("Warning"), tr("Could not find club %1 of runner %2 %3 in the club database!").arg(ref.toString()).arg(current_runner->getFirstName()).arg(current_runner->getLastName()));
				}
			}
			else if (stream.name() == "Club")
			{
				Club* club = clubDB.findClub(stream.attributes().value("name").toString());
				if (club)
					current_runner->addClub(club);
				else
					QMessageBox::warning(nullptr, tr("Warning"), tr("Could not find club %1 of runner %2 %3 in the club database!").arg(stream.attributes().value("name").toString()).arg(current_runner->getFirstName()).arg(current_runner->getLastName()));
			
			}
			else if (stream.name() == "Merge")
				merge_info.insert(std::make_pair(stream.attributes().value("from").toString().toInt(), stream.attributes().value("to").toString().toInt()));
		}
	
		if (current_runner)
			addRunner(current_runner, false);
	}
	
	// Also load the nicknames file
	CSVFile nick_file("etc/nicknames.txt");
	if (nick_file.open(true))
	{
		while (nick_file.nextLine())
			nicknames.push_back(std::make_pair(nick_file.getValue(0), nick_file.getValue(1)));
		
		nick_file.close();
	}	
	
	return true;
}

int RunnerDB::getFreeID()
{
	while (id_map.find(nextID) != id_map.end())
		++nextID;
	++nextID;
	return nextID - 1;
}
Runner* RunnerDB::getByID(int id)
{
	std::map<int, int>::iterator it = merge_info.find(id);
	while (it != merge_info.end())
	{
		id = it->second;
		it = merge_info.find(id);
	}
	
	std::map<int, Runner*>::iterator id_it = id_map.find(id);
	if (id_it == id_map.end())
		return nullptr;
	else
		return id_it->second;
}

void RunnerDB::addRunner(Runner* data, bool setNewID)
{
	assert(findRunner(*data) == nullptr && "Tried to add a runner who is already in the database!");
	
	int row = static_cast<int>(runners.size());
	int count = 1;
	
	beginInsertRows(QModelIndex(), row, row + count - 1);
	if (setNewID)
		data->setID(getFreeID());
	id_map.insert(std::make_pair(data->getID(), data));
	runners.push_back(data);
	endInsertRows();
}
void RunnerDB::deleteRunner(Runner* runner)
{
	size_t size = runners.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (runners[i] == runner)
		{
			removeRow(i);
			return;
		}
	}
}
Runner* RunnerDB::findRunner(const Runner& data)
{
	size_t size = runners.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (*runners[i] == data)
			return runners[i];
	}
	return nullptr;
}
Runner* RunnerDB::findRunner(const QString& first_name, const QString& last_name, int year, bool isMale)
{
	size_t size = runners.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (runners[i]->getFirstName() == first_name &&
			runners[i]->getLastName() == last_name &&
			runners[i]->getYear() == year &&
			runners[i]->isMale() == isMale)
			return runners[i];
	}
	return nullptr;
}
std::vector< Runner* > RunnerDB::findRunnersInClub(Club* club)
{
	std::vector< Runner* > result;
	size_t size = runners.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (runners[i]->isInClub(club))
			result.push_back(runners[i]);
	}
	return result;
}
inline bool testReplacement(QString to_find, QChar rep_1, QChar rep_2, const QString& src, const QString& dest)
{
	int test_pos = src.indexOf(to_find, 0, Qt::CaseInsensitive);
	if (test_pos >= 0)
	{
		QString test_string = src;
		test_string[test_pos] = rep_1;
		test_string[test_pos+1] = rep_2;
		if (test_string.compare(dest, Qt::CaseInsensitive) == 0)
			return true;
	}
	return false;
}
void RunnerDB::findSimilarRunners(Runner* runner, std::set< Runner* >& out)
{
	size_t size = nicknames.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (levenshteinDistanceSmaller(runner->getFirstName(), nicknames[i].first, 2))
		{
			QString old_name = runner->getFirstName();
			runner->setFirstName(nicknames[i].second);
			_findSimilarRunners(runner, out);
			runner->setFirstName(old_name);
		}
		if (levenshteinDistanceSmaller(runner->getFirstName(), nicknames[i].second, 2))
		{
			QString old_name = runner->getFirstName();
			runner->setFirstName(nicknames[i].first);
			_findSimilarRunners(runner, out);
			runner->setFirstName(old_name);
		}
	}
	
	_findSimilarRunners(runner, out);
}
void RunnerDB::_findSimilarRunners(Runner* runner, std::set< Runner* >& out)
{
	size_t size = runners.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (!levenshteinDistanceSmaller(runner->getFirstName(), runners[i]->getFirstName(), 2))
			continue;
		if (!levenshteinDistanceSmaller(runner->getLastName(), runners[i]->getLastName(), 2))
			continue;
		if (runner->getYear() > 0 && runners[i]->getYear() > 0 && runner->getYear() != runners[i]->getYear() &&
			!levenshteinDistanceSmaller(QString::number(runner->getYear()), QString::number(runners[i]->getYear()), 1))
			continue;
		
		// HARDCODED SPECIAL CASE: if two names are different in that one contains "jun" and the other one "sen", do not report them as similar
		if (testReplacement("jun", 's', 'e', runner->getFirstName(), runners[i]->getFirstName()))	continue;
		//if (testReplacement("jun", 's', 'e', runner->getLastName(), runners[i]->getLastName()))	continue;
		if (testReplacement("sen", 'j', 'u', runner->getFirstName(), runners[i]->getFirstName()))	continue;
		//if (testReplacement("sen", 'j', 'u', runner->getLastName(), runners[i]->getLastName()))	continue;
		
		out.insert(runners[i]);
	}
}
Runner* RunnerDB::getItem(const QModelIndex& index) const
{
	if (index.isValid())
	{
		Runner* item = static_cast<Runner*>(index.internalPointer());
		if (item)
			return item;
	}
	
	return nullptr;
}

void RunnerDB::clubsChangedInRow(int row)
{
	QModelIndex ind = index(row, 4);
	emit dataChanged(ind, ind);
}
void RunnerDB::changeClubForAllRunners(Club* src, Club* dest)
{
	size_t size = runners.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (runners[i]->isInClub(src))
		{
			runners[i]->removeClub(src);
			runners[i]->addClub(dest);
			clubsChangedInRow(i);
		}
	}
}

void RunnerDB::mergeRunners(Runner* src, Runner* dest)
{
	merge_info.insert(std::make_pair(src->getID(), dest->getID()));
	
	int num_clubs = src->getNumClubs();
	for (int i = 0; i < num_clubs; ++i)
		dest->addClub(src->getClub(i));
	
	//scoringDB.mergeRunners(src, dest);	// TODO: insert this as soon as runners are mentioned in scorings
	seriesScoringDB.mergeRunners(src, dest);
	
	runnerDB.deleteRunner(src);
}

int RunnerDB::rowCount(const QModelIndex& parent) const
{
	return runners.size();
}
int RunnerDB::columnCount(const QModelIndex& parent) const
{
	if (parent.isValid())
		return 0;	// the docs say that when implementing a table based model, columnCount() should return 0 when the parent is valid
	else
		return COLUMN_COUNT;
}
QModelIndex RunnerDB::index(int row, int column, const QModelIndex& parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();
	if (row < 0 || row >= static_cast<int>(runners.size()))
		return QModelIndex();
	if (column < 0 || column >= COLUMN_COUNT)
		return QModelIndex();
	
	return createIndex(row, column, runners[row]);
}
QVariant RunnerDB::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role != Qt::DisplayRole && role != Qt::EditRole)
		return QVariant();
	
	Runner* item = getItem(index);
	if (!item)
		return QVariant();
	
	if (index.column() == 0)
		return item->getFirstName();
	else if (index.column() == 1)
		return item->getLastName();
	else if (index.column() == 2)
		return item->isMale() ? tr("M") : tr("F");
	else if (index.column() == 3)
		return (item->getYear() > 0) ? QString::number(item->getYear()) : "";
	else if (index.column() == 4)
		return item->getClubListString(", ");
	else
		return QVariant();
}
QVariant RunnerDB::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		if (section == 0)
			return QVariant(tr("First name"));
		else if (section == 1)
			return QVariant(tr("Last name"));	
		else if (section == 2)
			return QVariant(tr("Gender"));
		else if (section == 3)
			return QVariant(tr("Year"));
		else if (section == 4)
			return QVariant(tr("Clubs"));
	}
	
	return QVariant();
}
Qt::ItemFlags RunnerDB::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return {};

	if (index.column() == 4)	// clubs
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	else
		return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool RunnerDB::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (role != Qt::EditRole)
		return false;
	
	Runner* item = getItem(index);
	if (!item)
		return false;
	
	if (index.column() == 0)
		item->setFirstName(value.toString());
	else if (index.column() == 1)
		item->setLastName(value.toString());
	else if (index.column() == 2)
	{
		if (value.toString() == tr("M"))
			item->setIsMale(true);
		else if (value.toString() == tr("F"))
			item->setIsMale(false);
		else
			return false;
	}
	else if (index.column() == 3)
		item->setYear(value.toInt());
	/*else if (index.column() == 4)
	{
		Club* new_club = clubDB.findClub(value.toString());
		if (new_club)
			item->setClub(new_club);
		else
			return false;
	}*/
	
	emit dataChanged(index, index);
	return true;
}

bool RunnerDB::insertRows(int row, int count, const QModelIndex& parent)
{
	if (row < 0 || row > static_cast<int>(runners.size()))
		return false;
	
	beginInsertRows(parent, row, row + count - 1);
	for (int r = 0; r < count; ++r)
	{
		Runner* new_runner = new Runner();
		new_runner->setID(getFreeID());
		id_map.insert(std::make_pair(new_runner->getID(), new_runner));
		runners.insert(runners.begin() + row, new_runner);
	}
	endInsertRows();
	
	return true;
}
bool RunnerDB::removeRows(int row, int count, const QModelIndex& parent)
{
	if (row < 0 || row + count > static_cast<int>(runners.size()))
		return false;
	
	beginRemoveRows(parent, row, row + count - 1);
	for (int r = row; r < row + count; ++r)
	{
		id_map.erase(runners[r]->getID());
		runners.erase(runners.begin() + r);
	}
	endRemoveRows();
	
	return true;
}

// ### RunnerTable ###

RunnerTable::RunnerTable()
{
	setEditTriggers(QAbstractItemView::AllEditTriggers);
	sortedModel = new QSortFilterProxyModel();
	sortedModel->setSourceModel(&runnerDB);
	sortedModel->setDynamicSortFilter(true);
	setModel(sortedModel);
	setSortingEnabled(true);
	sortByColumn(0, Qt::AscendingOrder);
	
	verticalHeader()->setVisible(false);
	
	resizeColumnsToContents();
	QHeaderView* headerView = horizontalHeader();
	headerView->setSectionResizeMode(0, QHeaderView::Stretch);
	headerView->setSectionResizeMode(1, QHeaderView::Stretch);
	headerView->setSectionResizeMode(4, QHeaderView::Stretch);
	headerView->setHighlightSections(false);
	
	// Gender Delegate
	QStringList items;
	items << tr("M") << tr("F");
	genderList = new QStringListModel(items);
	genderDelegate = new ComboBoxDelegate(genderList, 0);
	setItemDelegateForColumn(2, genderDelegate);
	
	// Club Delegate
	/*sortedClubModel = new QSortFilterProxyModel();
	sortedClubModel->setSourceModel(&clubDB);
	sortedClubModel->sort(0);
	clubDelegate = new ComboBoxDelegate(sortedClubModel, 0);
	setItemDelegateForColumn(4, clubDelegate);*/
	
	//connect(&runnerDB, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(sort()));
}
RunnerTable::~RunnerTable()
{
	setModel(nullptr);
	delete sortedModel;
	
	//delete clubDelegate;
	//delete sortedClubModel;
	
	delete genderDelegate;
	delete genderList;
}

void RunnerTable::sort()
{
	sortedModel->sort(sortedModel->sortColumn());
}
