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


#ifndef CSVFILE_H
#define CSVFILE_H

#include <vector>

#include <QString>
#include <map>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QFile;
class QTextStream;
class QWidget;
class QCheckBox;
class QLineEdit;
QT_END_NAMESPACE

/// Utility class to read CSV files
class CSVFile
{
public:
	
	enum FileError
	{
		NoError = 0,
		InvalidLine = 2
	};
	
	typedef std::vector<int> ColumnIndexes;
	typedef std::map< QString, ColumnIndexes > ColumnMap;
	
	CSVFile(const QString& path, const QChar _separator = ';', const QChar _stringDelimiter = '"', bool escape_minus = false);
	CSVFile(QString* string, const QChar _separator = ';', const QChar _stringDelimiter = '"', bool escape_minus = false);	// read / write to a string instead of a file
	~CSVFile();
	
	/// Shows a dialog with options for separator and delimiter characters and escaping of -
	bool showExportDialog(QWidget* parent = NULL);
	
	/// Opens the file and reads the first line (the column names) if the file is opened for reading
	bool open(bool for_reading);
	
	/// Closes the file
	void close();
	
	/// Returns if and which error ocurred
	inline FileError getError() {return error;}
	
	// Reading
	
	/// Returns the index of the column with the given name. The return value is negative if the column does not exist.
	/// Assumes that the column appears only one time, returns the first occurrence otherwise
	int getColumnIndex(const QString& name);
	
	/// Returns how often the column name appears in the file
	int getColumnCount(const QString& name);
	
	/// Returns the position of all occurrences of the given column name as vector
	std::vector<int>* getColumnVector(const QString& name);
	
	inline ColumnMap::iterator beginColumns() {return columnMap.begin();}
	inline ColumnMap::iterator endColumns() {return columnMap.end();}
	
	/// Tries to read the next line; returns false if at the end of file or an error ocurred. Use getError() to check this!
	bool nextLine();
	
	/// Use this after nextLine() returned true to read a value of a column; use getColumnIndex() to determine the index
	const QString& getValue(int index);
	
	// Writing
	
	void writeLine(const QStringList& values);
	
	// Getters / Setters
	
	QString getPath();
	
	inline QChar getSeparator() const {return separator;}
	inline void setSeparator(QChar value) {separator = value;}
	inline QChar getStringDelimiter() const {return stringDelimiter;}
	inline void setStringDelimiter(QChar value) {stringDelimiter = value;}
	inline bool getEscapeMinus() const {return escape_minus;}
	inline void setEscapeMinus(bool enable) {escape_minus = enable;}
	
private:
	
	bool processLine(const QString& line, std::vector< QString >& parts, bool checkColumnCount = true);
	
	QFile* file;
	QTextStream* stream;
	FileError error;
	QString* string;	// if reading from a string
	
	QChar separator;
	QChar stringDelimiter;
	bool escape_minus;
	
	ColumnMap columnMap;
	int num_columns; // number of original columns - this may be different from columnMap.size()!
	std::vector< QString > data;
};

class CSVExportDialog : public QDialog
{
Q_OBJECT
public:
	CSVExportDialog(QWidget* parent, CSVFile* file);
	
private slots:
	void exportClicked();
	
private:
	QLineEdit* separatorEdit;
	QLineEdit* stringDelimiterEdit;
	QCheckBox* escapeMinusCheck;
	
	CSVFile* file;
};

#endif
