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


#ifndef RESULTCSVIMPORT_H
#define RESULTCSVIMPORT_H

#include <QDialog>
#include <QMap>

#include "csvFile.h"

QT_BEGIN_NAMESPACE
class QGridLayout;
class QCheckBox;
class QComboBox;
class QSignalMapper;
QT_END_NAMESPACE

class CSVImportDialog : public QDialog
{
Q_OBJECT
public:
	
	CSVImportDialog(CSVFile* file, QWidget* parent = NULL);
	~CSVImportDialog();
	
	CSVFile::ColumnIndexes* colFirstName;
	CSVFile::ColumnIndexes* colLastName;
	CSVFile::ColumnIndexes* colYear;
	CSVFile::ColumnIndexes* colGender;
	CSVFile::ColumnIndexes* colTime;
	CSVFile::ColumnIndexes* colState;
	CSVFile::ColumnIndexes* colClubPart1;
	CSVFile::ColumnIndexes* colClubPart1Backup;
	CSVFile::ColumnIndexes* colClubPart2;
	CSVFile::ColumnIndexes* colClubPart2Backup;
	CSVFile::ColumnIndexes* colCategory;
	CSVFile::ColumnIndexes* colNotClassified;
	bool skip_empty_club_rows;
	QString maleString;
	
public slots:
	
	void abortClicked();
	void importClicked();
    void skipEmptyClubRowsChecked(bool checked);
    void columnChanged(int id);
	
private:
	
	void addRow(CSVFile::ColumnIndexes** id, QString desc, bool required, bool use_default, QString default_item, CSVFile::ColumnIndexes** backup_id = NULL);
	
	int current_row;
	QGridLayout* rows_layout;
	QCheckBox* skip_empty_club_rows_combo;
	QPushButton* import_button;
	
	QSignalMapper* signal_mapper;
	std::vector<CSVFile::ColumnIndexes**> id_vector;
	std::vector<QComboBox*> widget_vector;
	
	CSVFile* file;
};

#endif
