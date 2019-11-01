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


#include <time.h>

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QDir>
#include <QMessageBox>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include "mainWindow.h"
#include "location.h"
#include "club.h"
#include "runner.h"
#include "scoring.h"
#include "seriesScoring.h"
#include "global.h"

void loadProgramSettings();
void saveProgramSettings();

int main(int argc, char** argv)
{
	qsrand((uint)time(NULL));
	
	// Create application
	QApplication qapp(argc, argv);
	
	QTranslator translator;
	if (translator.load(QString("translations/" + QLocale::system().name())))
	//if (translator.load(QString("translations/de")))	// NOTE: comment out to test the German translation on a non-German system
		qapp.installTranslator(&translator);

	// Set settings defaults
	QCoreApplication::setOrganizationName("Thomas Schoeps");
	QCoreApplication::setApplicationName("OpenOrienteering");
	
	// Load program settings
	loadProgramSettings();
	
	// Load data
	if (!QDir("databases").exists())
		QDir().mkdir("databases");
	locationDB.loadFromFile();
	clubDB.loadFromFile();
	runnerDB.loadFromFile();

	// Let application run
	MainWindow* firstWindow = new MainWindow();
	firstWindow->show();
	
	int return_value;
	try
	{
		return_value = qapp.exec();
	}
	catch (...)
	{
		QString crash_dir = "crash_dump";
		QMessageBox::critical(NULL, qApp->translate("main", "Sorry!"), qApp->translate("main", "The program has crashed. It will now try to save all databases and all opened scorings in the \"%1\" subdirectory. You can try to use them by replacing the original ones with them.").arg(crash_dir));
		
		QDir curDir;
		curDir.mkdir(crash_dir);
		curDir = QDir(crash_dir);
		curDir.mkdir("databases");
		curDir.mkdir("my scorings");
		curDir.mkdir("my series scorings");
		
		crash_dir.append("/");
		
		locationDB.saveToFile(crash_dir);
		clubDB.saveToFile(crash_dir);
		runnerDB.saveToFile(crash_dir);
		scoringDB.saveToFile(crash_dir);
		seriesScoringDB.saveToFile(crash_dir);
		return 1;
	}
	
	// Save data
	locationDB.saveToFile();
	clubDB.saveToFile();
	runnerDB.saveToFile();
	scoringDB.saveToFile();
	seriesScoringDB.saveToFile();
	
	saveProgramSettings();
	
	return return_value;
}

void loadProgramSettings()
{
	if (!QDir("etc").exists())
		QDir().mkdir("etc");
	
	QFile program_file("etc/program.xml");
	if (!program_file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		for (int i = 0; i < 32; ++i)
		{
			int r = qrand() % 3;
			if (r == 0)
				installationKey[i] = 'A' + (qrand() % ('Z' - 'A' + 1));
			else if (r == 1)
				installationKey[i] = 'a' + (qrand() % ('z' - 'a' + 1));
			else if (r == 2)
				installationKey[i] = '0' + (qrand() % ('9' - '0' + 1));
		}
		installationKey[32] = 0;	
	}
	else
	{
		QXmlStreamReader stream(&program_file);
		while (!stream.atEnd())
		{
			stream.readNext();
			if (stream.tokenType() != QXmlStreamReader::StartElement)
				continue;
			
			if (stream.name() == "InstallationKey")
			{
				QString key = stream.attributes().value("value").toString();
				strcpy(installationKey, key.toLatin1());
			}
			else if (stream.name() == "NextIDValues")
			{
				locationDB.setNextID(stream.attributes().value("location").toString().toInt());
				clubDB.setNextID(stream.attributes().value("club").toString().toInt());
				runnerDB.setNextID(stream.attributes().value("runner").toString().toInt());
			}
		}
	}
	program_file.close();
}
void saveProgramSettings()
{
	QFile file("etc/program.xml");
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	
	QXmlStreamWriter stream(&file);
	stream.setAutoFormatting(true);
	stream.writeStartDocument();
	stream.writeStartElement("ProgramSettings");
	
	stream.writeEmptyElement("InstallationKey");
	stream.writeAttribute("value", installationKey);
	
	stream.writeEmptyElement("NextIDValues");
	stream.writeAttribute("location", QString::number(locationDB.getNextID()));
	stream.writeAttribute("club", QString::number(clubDB.getNextID()));
	stream.writeAttribute("runner", QString::number(runnerDB.getNextID()));
	
	stream.writeEndElement();
	stream.writeEndDocument();
}

#ifdef _MSC_VER

#include "Windows.h"
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(0, NULL);
}
#endif
