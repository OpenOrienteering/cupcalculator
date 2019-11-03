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


#include "util.h"

#include <QtGlobal>
#include <QChar>
#include <QCharRef>
#include <QList>
#include <QLocale>
#include <QStringList>

bool levenshteinDistanceSmaller(const QString& a, const QString& b, int threshold)
{
	if (qAbs(a.length() - b.length()) > threshold)
		return false;
	
	QString al = a.toLower();
	QString bl = b.toLower();
	int** v = new int*[al.length() + 1];
	for (int i = 0; i < al.length() + 1; ++i)
	{
		v[i] = new int[bl.length() + 1];
		v[i][0] = i;
	}
	for (int i = 1; i < bl.length() + 1; ++i)
		v[0][i] = i;
	
	for (int k = 1; k <= bl.length(); ++k)
	{
		for (int i = 1; i <= al.length(); ++i)
		{
			if (al[i-1] == bl[k-1])
				v[i][k] = v[i-1][k-1];
			else
				v[i][k] = qMin(qMin(v[i-1][k], v[i-1][k-1]), v[i][k-1]) + 1;
		}
	}
	
	bool smaller = v[al.length()][bl.length()] <= threshold;
	for (int i = 0; i < al.length() + 1; ++i)
		delete[] v[i];
	delete[] v;
	return smaller;
}

int parseTime(QString str)
{
	if (str.isEmpty())
		return 0;
	
	QStringList parts1 = str.split(":");
	if (parts1.size() > 3)
		return -1;
	else if (parts1.size() == 1)
	{
		// Only minutes
		bool ok = true;
		int result = str.toInt(&ok);
		return ok ? (60 * 100 * result) : -1;
	}
	
	QStringList parts2 = parts1[parts1.size() - 1].split(",");
	if (parts2.size() > 2)
		return -1;
	
	bool ok = true;
	int result = 0;
	
	// 1/10 seconds
	if (parts2.size() == 2)
	{
		result += 10 * parts2[1].toInt(&ok);
		if (!ok)
			return -1;
	}
	
	// seconds
	result += 100 * parts2[0].toInt(&ok);
	if (!ok)
		return -1;
	
	// minutes
	result += 60 * 100 * parts1[parts1.size() - 2].toInt(&ok);
	if (!ok)
		return -1;
	
	// hours
	if (parts1.size() == 3)
	{
		result += 60 * 60 * 100 * parts1[0].toInt(&ok);
		if (!ok)
			return -1;
	}
	
	return result;
}
QString timeToString(int t)
{
	int hundreds = t % 100;
	t /= 100;
	int seconds = t % 60;
	t /= 60;
	int minutes = t;
	
	if (hundreds > 0)
		return QString("%1:%2,%3").arg(minutes, 0, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')).arg(hundreds, 2, 10, QChar('0'));
	else
		return QString("%1:%2").arg(minutes, 0, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

QString pointsToString(int points, int decimal_places, int decimal_factor)
{
	bool negative = points < 0;
	if (negative)
		points = -points;
	
	int hundreds = points % decimal_factor;
	points /= decimal_factor;
	
	QString number;
	if (decimal_places == 0)
		number = QString("%1").arg(points, 0, 10, QChar('0'));
	else
		number = QString("%1" + QString(QLocale().decimalPoint()) + "%2").arg(points, 0, 10, QChar('0')).arg(hundreds, decimal_places, 10, QChar('0'));
	
	if (!negative)
		return number;
	else
		return "(" + number + ")";
}

DoubleValidator::DoubleValidator(double bottom, double top, int decimals) : QDoubleValidator(bottom, top, decimals, nullptr)
{
	// Don't cause confusion, accept only English formatting
	setLocale(QLocale(QLocale::English));
}
QValidator::State DoubleValidator::validate(QString& input, int& pos) const
{
	// Transform thousands group separators into decimal points to avoid confusion
	input.replace(',', '.');
	
	// Do not return QValidator::Intermediate
	return (QDoubleValidator::validate(input, pos) == QValidator::Acceptable) ? QValidator::Acceptable : QValidator::Invalid;
}
