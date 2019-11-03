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


#ifndef CUPCALCULATOR_UTIL_H
#define CUPCALCULATOR_UTIL_H

#include <QDoubleValidator>
#include <QString>
#include <QValidator>

/// Checks if the Levenshtein distance of the two strings is below a threshold, case insensitive
bool levenshteinDistanceSmaller(const QString& a, const QString& b, int threshold);

/// Parses time formats as output by Sportsoftware and returns the number of 1/100 seconds
int parseTime(QString str);
QString timeToString(int t);

/// Converts a fixed-comma-position int to a string, appends zeros if neccessary
QString pointsToString(int points, int decimal_places, int decimal_factor);

/// Fixed-point number with 2 decimal places
class FPNumber
{
public:
	
	inline FPNumber() {}
	inline FPNumber(double v) {setValue(v);}
	inline FPNumber(int v) {value = v;}
	
	inline void setValue(double v) {value = (int)(v * 100 + 0.5);}
	
	inline int toInt() const {return value;}
	inline double toDouble() const {return value * 0.01;}
	inline QString toString() const {return QString::number(toDouble());}
	
	inline void operator= (const double v) {setValue(v);}
	
private:
	
	int value;
};

/// Double validator for line edit widgets
class DoubleValidator : public QDoubleValidator
{
public:
	
	DoubleValidator(double bottom, double top = 10e10, int decimals = 20);
	
	virtual State validate(QString& input, int& pos) const;
};

#endif
