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


#include "csvFile.h"

#include "assert.h"

#include <QtWidgets>
#include <QFile>
#include <QTextStream>
#include <QStringList>

CSVFile::CSVFile(const QString& path, const QChar _separator, const QChar _stringDelimiter, bool escape_minus)
{
	stream = nullptr;
	file = new QFile(path);

	error = NoError;
	separator = _separator;
	stringDelimiter = _stringDelimiter;
	this->escape_minus = escape_minus;
}
CSVFile::CSVFile(QString* string, const QChar _separator, const QChar _stringDelimiter, bool escape_minus)
{
	stream = nullptr;
	file = nullptr;
	this->string = string;
	
	error = NoError;
	separator = _separator;
	stringDelimiter = _stringDelimiter;
	this->escape_minus = escape_minus;
}
CSVFile::~CSVFile()
{
	close();
	
	delete stream;
	delete file;
}

bool CSVFile::showExportDialog(QWidget* parent)
{
	CSVExportDialog dialog(parent, this);
	dialog.setWindowModality(Qt::WindowModal);
	dialog.exec();
	return (dialog.result() == QDialog::Accepted);
}

bool CSVFile::open(bool for_reading)
{
	if (file)
	{
		// Open file
		if (!file->open((for_reading ? QIODevice::ReadOnly : QIODevice::WriteOnly) | QIODevice::Text))
			return false;
		stream = new QTextStream(file);
		if (for_reading && stream->atEnd())
			return false;
	}
	else
	{
		stream = new QTextStream(string, (for_reading ? QIODevice::ReadOnly : QIODevice::WriteOnly) | QIODevice::Text);
	}
	
	// TODO: this is ok as long as only the German Sportsoftware is supported
	stream->setCodec("latin-1");
	
	if (for_reading)
	{
		// Read first line (column names)
		QString line = stream->readLine();
		std::vector< QString > columns;
		processLine(line, columns, false);
		num_columns = columns.size();
		
		int size = columns.size();
		for (int i = 0; i < size; ++i)
		{
			ColumnMap::iterator it = columnMap.find(columns[i]);
			if (it == columnMap.end())
			{
				std::vector<int> new_vector;
				new_vector.push_back(i);
				columnMap.insert(std::make_pair(columns[i], new_vector));
			}
			else
			{
				std::vector<int> new_vector = it->second;
				new_vector.push_back(i);
				//qDebug("%s %d %d\n", columns[i].toStdString().c_str(), new_vector[new_vector.size() - 2], i);
				columnMap.erase(it);
				columnMap.insert(std::make_pair(columns[i], new_vector));
			}
		}
	}
	
	return true;
}

void CSVFile::close()
{
	if (file)
		file->close();
}

int CSVFile::getColumnIndex(const QString& name)
{
	ColumnMap::iterator it = columnMap.find(name);
	if (it == columnMap.end())
		return -1;
	else
		return it->second[0];
}
int CSVFile::getColumnCount(const QString& name)
{
	ColumnMap::iterator it = columnMap.find(name);
	if (it == columnMap.end())
		return -1;
	else
		return it->second.size();
}
std::vector< int >* CSVFile::getColumnVector(const QString& name)
{
	ColumnMap::iterator it = columnMap.find(name);
	if (it == columnMap.end())
		return nullptr;
	else
		return &it->second;
}

bool CSVFile::nextLine()
{
	QString line;
	do
	{
		if (stream->atEnd())
			return false;
		
		line = stream->readLine();
	} while (line.isEmpty());
	
	processLine(line, data);
	return true;
}

const QString& CSVFile::getValue(int index)
{
	return data[index];
}

void CSVFile::rewind()
{
	stream->seek(0);
	stream->readLine();
}

void CSVFile::renameColumn(const QString& oldName, const QString& newName)
{
	ColumnMap::iterator oldIt = columnMap.find(oldName);
	if (oldIt == columnMap.end())
		return;
	ColumnMap::iterator newIt = columnMap.find(newName);
	if (newIt == columnMap.end())
		newIt = columnMap.insert(std::make_pair(newName, std::vector<int>())).first;
	for (size_t i = 0; i < oldIt->second.size(); ++ i)
		newIt->second.push_back(oldIt->second.at(i));
	columnMap.erase(oldIt);
}

void CSVFile::deleteColumn(const QString& name)
{
	ColumnMap::iterator it = columnMap.find(name);
	if (it == columnMap.end())
		return;
	columnMap.erase(it);
}

QString CSVFile::getPath()
{
	if (file)
		return file->fileName();
	else
		return "-";
}

bool CSVFile::processLine(const QString& line, std::vector< QString >& parts, bool checkColumnCount)
{
	QString cur_value = "";
	bool reading_string = false;
	
	parts.clear();
	
	int size = line.size();
	for (int i = 0; i < size; ++i)
	{
		QChar c = line[i];
		if (!reading_string && c == separator)
		{
			parts.push_back(cur_value);
			cur_value.clear();
		}
		else if (c == stringDelimiter)
		{
			reading_string = !reading_string;
		}
		else if (c == '\\')
		{
			if (line[i+1] == stringDelimiter)
			{
				cur_value += line[i+1];
				++i;
			}
			else if (line[i+1] == '\\')
			{
				cur_value += line[i+1];
				++i;
			}
			else
				cur_value += c;
		}
		else
		{
			cur_value += c;
		}
	}
	
	parts.push_back(cur_value);
	
	if (reading_string)
	{
		error = InvalidLine;
		return false;
	}
	
	// Make sure we have the correct number of columns
	if (checkColumnCount && num_columns != (int)parts.size())
	{
		while ((int)parts.size() < num_columns)
			parts.push_back("");
		while ((int)parts.size() > num_columns)
			parts.pop_back();
	}
	
	return true;
}

void CSVFile::writeLine(const QStringList& values)
{
	QString text = QString();
	
	for (int i = 0; i < values.size(); ++i)
	{
		// NOTE: if the values contain the string delimiter, it is simply removed
		QString value = values[i];
		value.remove(stringDelimiter);
		
		// Special case because LibreOffice interprets "-" alone in a strange way and displays "-0" with default import options
		if (escape_minus && value == "-")
			value = "'-";
		
		if (values[i].contains(separator))
			text += stringDelimiter + value + stringDelimiter;
		else
			text += value;
		
		if (i < values.size() - 1)
			text += separator;
	}
	text += "\n";
	
	*stream << text;
}

// ### CSVExportDialog ###

CSVExportDialog::CSVExportDialog(QWidget* parent, CSVFile* file) : QDialog(parent), file(file)
{
	setWindowTitle(tr("CSV export options"));
	
	QLabel* separator_label = new QLabel(tr("Separator character:"));
	separatorEdit = new QLineEdit(file->getSeparator());
	separatorEdit->setMaxLength(1);
	
	QLabel* string_delimiter_label = new QLabel(tr("String delimit character:"));
	stringDelimiterEdit = new QLineEdit(file->getStringDelimiter());
	stringDelimiterEdit->setMaxLength(1);
	
	escapeMinusCheck = new QCheckBox(tr("Escape - with '"));
	escapeMinusCheck->setChecked(file->getEscapeMinus());
	
	QPushButton* abort_button = new QPushButton(tr("Abort"));
	QPushButton* export_button = new QPushButton(QIcon("images/save.png"), tr("Export"));
	
	QHBoxLayout* buttons_layout = new QHBoxLayout();
	buttons_layout->addWidget(abort_button);
	buttons_layout->addWidget(export_button);
	
	QGridLayout* layout = new QGridLayout();
	layout->addWidget(separator_label, 0, 0);
	layout->addWidget(separatorEdit, 0, 1);
	layout->addWidget(string_delimiter_label, 1, 0);
	layout->addWidget(stringDelimiterEdit, 1, 1);
	layout->addWidget(escapeMinusCheck, 2, 0, 1, 2);
	layout->addLayout(buttons_layout, 3, 0, 1, 2);
	setLayout(layout);
	
	connect(abort_button, SIGNAL(clicked(bool)), this, SLOT(reject()));
	connect(export_button, SIGNAL(clicked(bool)), this, SLOT(exportClicked()));
}

void CSVExportDialog::exportClicked()
{
	if (separatorEdit->text().length() < 1)
	{
		QMessageBox::warning(this, tr("Error"), tr("Please enter a separator character!"));
		return;
	}
	if (stringDelimiterEdit->text().length() < 1)
	{
		QMessageBox::warning(this, tr("Error"), tr("Please enter a string delimit character!"));
		return;
	}
	
	file->setSeparator(separatorEdit->text()[0]);
	file->setStringDelimiter(stringDelimiterEdit->text()[0]);
	file->setEscapeMinus(escapeMinusCheck->isChecked());
	
	accept();
}
