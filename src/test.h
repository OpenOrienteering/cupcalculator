/*
 *    Copyright 2011 Thomas Schöps
 *    
 *    This file is part of OpenOrienteering's scoring tool.
 * 
 *    OpenOrienteering's scoring tool is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 * 
 *    OpenOrienteering's scoring tool is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 * 
 *    You should have received a copy of the GNU General Public License
 *    along with OpenOrienteering's scoring tool.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef TEST_H
#define TEST_H

// Somehow including QtGui like suggested in the documentation did not work to choose the GUI variant of the QTEST_MAIN macro, so define this manually here ...
#define QT_GUI_LIB
#include <QtTest/QtTest>

class ResultList;

class TestScoringTool: public QObject
{
Q_OBJECT
private slots:
	void fixedPointCalculation();
	void timeRatioCalculation();
	
private:
	void checkResults(ResultList* output_list, const QString& oracle);
};

#endif
