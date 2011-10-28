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


#ifndef CLUBDIALOG_H
#define CLUBDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QTreeView;
class QLabel;
QT_END_NAMESPACE

class Club;
class Location;

class ClubDialog : public QDialog
{
Q_OBJECT
public:
	
	ClubDialog(Club* _club, QWidget* parent = NULL);
	~ClubDialog();
	
	// Overridden methods
	virtual void closeEvent(QCloseEvent* event);
	
public slots:
	
	void nameChanged();
	void addCountryClicked();
	void addStateClicked();
	void removeLocationClicked();
	void clearSelectionClicked();
	void locationSelected();
	void setLocationText();
	void updateActions();
		
private:
	
	void selectLocation(Location* location);
	void insertRow(const QString& text, bool second_level);
	void updateWindowTitle();
	
	QLineEdit* nameEdit;
	QLabel* regionLabel;
	QTreeView* locationView;
	QPushButton* addCountryButton;
	QPushButton* addStateButton;
	QPushButton* removeLocationButton;
	QPushButton* clearSelectionButton;
	
	Club* club;
	QString old_name;
};

#endif
