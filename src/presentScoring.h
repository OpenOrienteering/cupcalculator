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


#ifndef CUPCALCULATOR_PRESENTSCORING_H
#define CUPCALCULATOR_PRESENTSCORING_H

#include <map>

#include <QDialog>

class Layout;
class AbstractCategory;
class ResultList;
class Event;
class PresentScoringController;
class Scoring;

QT_BEGIN_NAMESPACE
class QListWidget;
class QComboBox;
class QCheckBox;
class QTranslator;
class QLineEdit;
QT_END_NAMESPACE

class PresentationWidget : public QWidget
{
Q_OBJECT
public:
	
	PresentationWidget(PresentScoringController* controller, Layout* layout, QSize size, bool fullscreen, QWidget* screen_info_widget, QWidget* parent = nullptr);
	~PresentationWidget();
	
	void showResultList(ResultList* list, int show_first_x_runners = -1);
	void setPage(int page_number, int num_pages);
	
    virtual void paintEvent(QPaintEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
	
    virtual void resizeEvent(QResizeEvent* event);
    virtual void closeEvent(QCloseEvent* event);
	
public slots:
	
	void nextAnimationFrame();
	
private:
	
	QString relayTextForItem(int row, int column, ResultList* list, bool& out_bold, int num_runners_per_result);
	QString textForItem(int row, int column, ResultList* list, bool& out_bold);
	QPixmap* createResultListPixmap(ResultList* list, bool calc_delta_scroll = false);
	void drawCurrentSlide();
	
	float offset_x, offset_y;
	float display_width, display_height;
	
	float max_scroll, delta_scroll, current_scroll_target, current_scroll, previous_scroll;
	
	ResultList* current_list;
	QPixmap* current_list_img;
	QPixmap* current_slide;
	float current_slide_alpha;
	bool current_slide_dirty;
	int show_first_x_runners;
	
	ResultList* previous_list;
	QPixmap* previous_list_img;
	QPixmap* previous_slide;
	float previous_slide_alpha;
	
	int page_number, num_pages;
	
	int interval;		// in milliseconds
	int elapsed_time;	// in milliseconds
	PresentScoringController* controller;
	Layout* layout;
};

class PresentScoringController
{
public:
	
	PresentScoringController(Scoring* scoring, Layout* layout, QString language, ResultList* result_list, int event_year, bool show_team_scoring, bool uncover_team_scoring_gradually,
							 bool show_intermediate_team_scoring, int show_first_x_runners, bool no_runner_points, bool include_non_scoring_runners,
							 const std::vector< AbstractCategory* >& category_vector, QWidget* window);
	~PresentScoringController();
	
	void setPage(int page);
	
	void start();
	void mousePressEvent(QMouseEvent* event);
	bool keyPressEvent(QKeyEvent* event);
	
private:
	
	enum PageType
	{
		PageType_SingleResults = 0,
		PageType_TeamResults = 1
	};
	
	struct PresentationPage
	{
		PageType type;
		ResultList* list;
	};
	
	QString translateWith(QTranslator* translator, const char* string);
	
	int current_page;
	std::vector<PresentationPage> pages;
	
	Scoring* scoring;
	Layout* layout;
	ResultList* result_list;
	bool show_team_scoring;
	bool show_intermediate_team_scoring;
	int show_first_x_runners;
	std::vector<AbstractCategory*> category_vector;
	
	PresentationWidget* widget;
};

class PresentScoringDialog : public QDialog
{
Q_OBJECT
public:
	
	PresentScoringDialog(Event* event, int event_year, ResultList* result_list, QWidget* parent = nullptr);
	~PresentScoringDialog();
	
public slots:
	
	void currentScoringChanged(int index);
	void presentClicked();
    void teamScoringChecked(bool checked);
    void showOnlyFirstXChecked(bool checked);
    void includeNonScoringRunnersClicked(bool checked);
	
private:
	
	void recalculateCategories();
	
	QComboBox* scoringCombo;
	QCheckBox* teamScoringCheck;
	QCheckBox* teamScoringStepByStepCheck;
	QCheckBox* teamScoringCategoryCheck;
	QCheckBox* showOnlyFirstXCheck;
	QLineEdit* showOnlyFirstXEdit;
	QCheckBox* doNotShowRunnerPointsCheck;
	QCheckBox* includeNonScoringRunnersCheck;
	
	QListWidget* categoryList;
	QListWidget* otherCategoryList;
	
	QComboBox* layoutCombo;
	QComboBox* languageCombo;
	
	std::map<int, AbstractCategory*> category_map;
	
	Event* event;
	int event_year;
	ResultList* result_list;
};

#endif
