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


#ifndef CUPCALCULATOR_PROBLEMWIDGET_H
#define CUPCALCULATOR_PROBLEMWIDGET_H

#include <vector>

#include <QtGlobal>
#include <QDialog>
#include <QItemDelegate>
#include <QObject>
#include <QSize>
#include <QString>
#include <QStyleOptionViewItem>
#include <QTableWidget>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QModelIndex;
class QStyleOptionViewItem;
class QWidget;
QT_END_NAMESPACE

class ActionSelectionDelegate;

class Problem : public QObject
{
Q_OBJECT
public:
	
	enum SolutionType
	{
		SolutionType_No = 0,
		SolutionType_Keep,
		SolutionType_Change
	};
	
	Problem();

	virtual int getNumSolutions() = 0;
	virtual QString getSolutionDescription(int i) = 0;
	virtual void applySolution(int i) = 0;
	
	virtual void setToSolutionType(SolutionType type) {}
	
	inline const QString& getDescription() const {return description;}
	inline int getSelection() const {return selection;}
	inline void setSelection(int s) {selection = s;}
	
protected:
	
	int selection;
	QString description;
};

class ProblemWidget : public QTableWidget
{
Q_OBJECT
public:
	
	ProblemWidget(QWidget* parent = nullptr);
    virtual ~ProblemWidget();
	
	void addProblem(Problem* problem);
	
	void applyAllSolutions();
	void updateAllSolutions();
	void showAsDialog(const QString& title, QWidget* parent);
	
	inline int problemCount() const {return problems.size();}
	
    virtual QSize sizeHint() const;
	
public slots:
	
	void setAllToNo();
	void setAllToKeep();
	void setAllToChange();
	
private:
	
	std::vector<Problem*> problems;
	ActionSelectionDelegate* actionSelectionDelegate;
};
class ProblemDialog : public QDialog
{
Q_OBJECT
public:
	ProblemDialog(const QString& title, ProblemWidget* widget, QWidget* parent = nullptr);
public slots:
	void okClicked();
private:
	ProblemWidget* widget;
};

class ActionSelectionDelegate : public QItemDelegate
{
Q_OBJECT
public:
	
	ActionSelectionDelegate(QObject *parent = nullptr);
	
	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
	virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	
private slots:
	
	void emitCommitData();
	
private:
	
};

#endif
