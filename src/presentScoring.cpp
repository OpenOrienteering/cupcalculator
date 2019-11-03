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


#include "presentScoring.h"

#include <assert.h>

#include <QtWidgets>

#include "event.h"
#include "scoring.h"
#include "resultList.h"
#include "config.h"
#include "runner.h"
#include "layout.h"

#define TRANSLATION_1 QT_TRANSLATE_NOOP("presentation", "Intermediate team results");
#define TRANSLATION_2 QT_TRANSLATE_NOOP("presentation", "Final team results");

// ### PresentationWidget ###

PresentationWidget::PresentationWidget(PresentScoringController* controller, Layout* layout, QSize size, bool fullscreen, QWidget* screen_info_widget, QWidget* parent)
 : QWidget(parent), controller(controller)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(tr("Presentation"));
	setWindowIcon(QIcon("images/control.png"));
	
	setAutoFillBackground(false);
	
	this->layout = new Layout(*layout);
	
	page_number = 0;
	num_pages = 0;
	
	elapsed_time = 0;
	
	current_list = nullptr;
	current_list_img = nullptr;
	current_slide = nullptr;
	current_slide_alpha = 0;
	current_slide_dirty = true;
	previous_list = nullptr;
	previous_list_img = nullptr;
	previous_slide = nullptr;
	previous_slide_alpha = 0;
	
	// NOTE: Members must be initialized before calling show(), as this will call resizeEvent()
	if (!fullscreen)
	{
		setGeometry(0, 0, size.width(), size.height());
		show();
		int screenId = QApplication::desktop()->screenNumber(screen_info_widget->geometry().center());
		//QWidget* screen = QApplication::desktop()->screen(screenId);
		//if (QApplication::desktop()->isVirtualDesktop()) {
			QRect srect(0, 0, size.width(), size.height());
			move(QApplication::desktop()->availableGeometry(screenId).center() - srect.center());
		//}
	}
	else
		showFullScreen();
	
	QTimer* timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(nextAnimationFrame()));
	interval = (int)(1000 / 25.0f);
	timer->start(interval);
}
PresentationWidget::~PresentationWidget()
{
	delete layout;
	delete current_list_img;
	delete current_slide;
	delete previous_list_img;
	delete previous_slide;
}

void PresentationWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter;
	painter.begin(this);
	painter.setClipRect(event->rect());
	
	// Black borders
	if (offset_x > 0)
	{
		painter.fillRect(0, 0, offset_x, height(), qRgb(0, 0, 0));
		painter.fillRect(width() - offset_x, 0, offset_x, height(), qRgb(0, 0, 0));
	}
	else if (offset_y > 0)
	{
		painter.fillRect(0, 0, width(), offset_y, qRgb(0, 0, 0));
		painter.fillRect(0, height() - offset_y, width(), offset_y, qRgb(0, 0, 0));
	}
	
	// Slide(s)
	if (current_slide_dirty)
	{
		drawCurrentSlide();
		current_slide_dirty = false;
	}
	
	if (previous_slide_alpha > 0)
	{
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.setOpacity(1);
		painter.drawPixmap(QPoint(offset_x, offset_y), *previous_slide);
	}
	if (current_slide_alpha > 0)
	{
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.setOpacity((previous_slide_alpha > 0) ? current_slide_alpha : 1);
		painter.drawPixmap(QPoint(offset_x, offset_y), *current_slide);
	}
	
	/*painter.fillRect(offset_x, offset_y, width() - 2*offset_x, height() - 2*offset_y, qRgb(255, 255, 255));
	// Result lists
	if (previous_slide_alpha > 0)
		paintList(previous_list_img, previous_slide_alpha, previous_scroll, painter);
	if (current_slide_alpha > 0)
		paintList(current_list_img, current_slide_alpha, current_scroll, painter);*/
	
	painter.end();
}
void PresentationWidget::mousePressEvent(QMouseEvent* event)
{
    controller->mousePressEvent(event);
}
void PresentationWidget::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape)
		close();
	else
	{
		if (!controller->keyPressEvent(event))
			QWidget::keyPressEvent(event);
	}
}
void PresentationWidget::wheelEvent(QWheelEvent* event)
{
	if (event->orientation() == Qt::Vertical)
	{
		float degrees = event->delta() / 8.0f;
		float numSteps = degrees / 15.0f;
		float delta = -numSteps * delta_scroll;
		
		if ((current_scroll_target - current_scroll) * delta < 0)
			current_scroll_target = current_scroll;
		else
		{
			current_scroll_target += delta;
			if (current_scroll_target < 0)
				current_scroll_target = 0;
			else if (current_scroll_target > max_scroll)
				current_scroll_target = max_scroll;
		}
		
		event->accept();
	}
}

void PresentationWidget::resizeEvent(QResizeEvent* event)
{
	delete current_slide;
	delete current_list_img;
	delete previous_slide;
	delete previous_list_img;
	previous_list_img = nullptr;
	
	const float aspect = 4 / 3.0f;
	if (height() * aspect >= width())
	{
		// black top and bottom
		display_width = width();
		display_height = width() / aspect;
		offset_y = 0.5f * (height() - display_height);
		offset_x = 0;
	}
	else
	{
		// black left and right
		display_width = height() * aspect;
		display_height = height();
		offset_y = 0;
		offset_x = 0.5f * (width() - display_width);
	}
	
	current_slide = new QPixmap(display_width, display_height);
	current_slide_dirty = true;
	current_list_img = createResultListPixmap(current_list, true);
	if (current_list_img)
	{
		Layout::Rect* list_rect = layout->getRectByID("content");
		if (list_rect)
			max_scroll = qMax(qreal(0), (current_list_img->height() - list_rect->rect.height() * display_height) / display_height);
		else
			max_scroll = 0;
	}
	else
		max_scroll = 0;
	
	previous_slide = new QPixmap(display_width, display_height);
	previous_slide_alpha = 0;
}
void PresentationWidget::closeEvent(QCloseEvent* event)
{
	delete controller;
	event->accept();
}

void PresentationWidget::drawCurrentSlide()
{
	// Update items
	Layout::Point* title_point = layout->getPointByID("title");
	if (title_point)
	{
		title_point->text = current_list->getTitle();
		title_point->strong_font = true;
	}
	
	if (num_pages > 0)
	{
		Layout::Point* pages_point = layout->getPointByID("page_no");
		if (pages_point)
			pages_point->text = QString::number(page_number) + " / " + QString::number(num_pages);
	}
	
	// Draw layout
	QPainter painter(current_slide);
	layout->draw(&painter, display_width, display_height);
	
	// Draw list content
	Layout::Rect* list_rect = layout->getRectByID("content");
	if (list_rect && current_list_img)
	{
		QRectF real_list_rect(list_rect->rect.left() * display_width, list_rect->rect.top() * display_height, list_rect->rect.width() * display_width, list_rect->rect.height() * display_height);
		
		painter.save();
		painter.setClipRect(real_list_rect, Qt::IntersectClip);
		
		QPointF position = QPointF(real_list_rect.center().x() - 0.5f * current_list_img->width(), real_list_rect.top() - current_scroll * display_height);
		painter.drawPixmap(position, *current_list_img);
		
		painter.restore();
	}
}

QPixmap* PresentationWidget::createResultListPixmap(ResultList* list, bool calc_delta_scroll)
{
	if (!list)
		return nullptr;
	if (list->rowCount() <= 0)
		return nullptr;
	Layout::Rect* list_rect = layout->getRectByID("content");
	if (!list_rect)
		return nullptr;
	
	QPixmap* result = nullptr;
	
	QFont font_normal = layout->qFontFromFont(list_rect->font_normal, display_height);
	QFont font_strong = layout->qFontFromFont(list_rect->font_strong, display_height);
	QFontMetricsF font_metrics_normal(font_normal);
	QFontMetricsF font_metrics_strong(font_strong);
	
	const float COLUMN_SEPARATION_DIST = display_height / 30.0f;
	float ROW_HEIGHT = font_metrics_normal.height() * list_rect->font_normal->line_spacing;
	float list_width = 0;
	
	if (calc_delta_scroll)
		delta_scroll = 3 * ROW_HEIGHT / display_height;
	
	int num_runners_per_result = 1;
	if (list->getLastRunnerColumn() != list->getFirstRunnerColumn())
	{
		for (int r = 0; r < list->rowCount(); ++r)
		{
			int runners_this_row = 0;
			for (int c = list->getFirstRunnerColumn(); c <= list->getLastRunnerColumn(); c += 2)
			{
				if (list->getData(r, c).value<void*>() != nullptr)
					++runners_this_row;
			}
			if (runners_this_row > num_runners_per_result)
				num_runners_per_result = runners_this_row;
			if (num_runners_per_result == ((list->getLastRunnerColumn() - list->getFirstRunnerColumn()) / 2) + 1)
				break;	// reached maximum
		}
	}
	bool relay = num_runners_per_result > 1;
	int num_entries = (show_first_x_runners < 0) ? list->rowCount() : qMin(show_first_x_runners, list->rowCount());
	// Dimensions of output:
	int num_rows = relay ? (num_entries * (1 + num_runners_per_result)) : num_entries;
	int num_columns = relay ? ((list->getPointsColumn() >= 0) ? 4 : 3) : list->columnCount();
	
	float* column_offsets = new float[num_columns];
	float* column_widths = new float[num_columns];
	
	for (int c = 0; c < num_columns; ++c)
	{
		column_offsets[c] = list_width;
		
		float max_width = 0;
		for (int r = 0; r < num_rows; ++r)
		{
			bool strong;
			QString text;
			if (relay)
				text = relayTextForItem(r, c, list, strong, num_runners_per_result);
			else
				text = textForItem(r, c, list, strong);
			float text_width;
			
			if (!strong)
				text_width = font_metrics_normal.width(text);
			else
				text_width = font_metrics_strong.width(text);
			
			if (text_width > max_width)
				max_width = text_width;
		}
		
		column_widths[c] = max_width;
		list_width += max_width;
		if (c < num_columns - 1)
			list_width += (list->getColumnType(c) == ResultList::ColumnRank) ? (0.5f * COLUMN_SEPARATION_DIST) : COLUMN_SEPARATION_DIST;
	}
	float list_height = num_rows * ROW_HEIGHT;
	
	// List too broad? Shrink the club column ... NOTE: This assumes that shrinking this one is enough and does not result in a negative or very small column width!
	float max_allowed_width = list_rect->rect.width() * display_width;
	int shrinked_column = -1;
	if (list_width > max_allowed_width)
	{
		/*float max_column_width = column_widths[0];
		shrinked_column = 0;
		for (int c = 1; c < list->columnCount(); ++c)
		{
			if (column_widths[c] < max_column_width)
				continue;
			
			max_column_width = column_widths[c];
			shrinked_column = c;
		}*/
		shrinked_column = relay ? 1 : list->getClubColumn();
		
		float shrink_amount = list_width - max_allowed_width;
		column_widths[shrinked_column] -= shrink_amount;
		for (int c = shrinked_column + 1; c < list->columnCount(); ++c)
			column_offsets[c] -= shrink_amount;
		
		list_width = max_allowed_width;
	}
	
	result = new QPixmap(list_width, list_height);
	result->fill(Qt::transparent);
	QPainter painter(result);
	
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	painter.setBrush(Qt::NoBrush);
	painter.setPen(qRgb(0, 0, 0));
	
	for (int c = 0; c < num_columns; ++c)
	{
		Qt::Alignment alignment;
		if (relay)
		{
			if (c == 0)
				alignment = Qt::AlignRight;
			else if (c == 1)
				alignment = Qt::AlignLeft;
			else if (c == 2)
				alignment = Qt::AlignRight;
			else if (c == 3)
				alignment = Qt::AlignRight;
		}
		else
		{
			ResultList::ColumnType type = list->getColumnType(c);
			if (type == ResultList::ColumnRank)			alignment = Qt::AlignRight;
			else if (type == ResultList::ColumnRunner)	alignment = Qt::AlignLeft;
			else if (type == ResultList::ColumnClub)	alignment = Qt::AlignHCenter;
			else if (type == ResultList::ColumnTime)	alignment = Qt::AlignRight;
			else if (type == ResultList::ColumnPoints)	alignment = Qt::AlignRight;
			else										alignment = Qt::AlignRight;
		}
		
		for (int r = 0; r < num_rows; ++r)
		{
			bool strong;
			QString text;
			if (relay)
				text = relayTextForItem(r, c, list, strong, num_runners_per_result);
			else
				text = textForItem(r, c, list, strong);
			
			QRectF text_rect = QRectF(column_offsets[c], r * ROW_HEIGHT, column_widths[c], ROW_HEIGHT);
			if (strong)
			{
				painter.setFont(font_strong);
				painter.setPen(list_rect->font_strong->color);
			}
			else
			{
				painter.setFont(font_normal);
				painter.setPen(list_rect->font_normal->color);
			}
			
			if (shrinked_column == c)
			{
				QFontMetricsF& metrics = strong ? font_metrics_strong : font_metrics_normal;
				painter.drawText(text_rect, alignment, metrics.elidedText(text, Qt::ElideRight, column_widths[c]));
			}
			else
				painter.drawText(text_rect, alignment, text);
		}
	}
	
	delete[] column_offsets;
	
	return result;
}
QString PresentationWidget::relayTextForItem(int row, int column, ResultList* list, bool& out_bold, int num_runners_per_result)
{
	int entry_line = row / (1 + num_runners_per_result);
	int result_line = row % (1 + num_runners_per_result);
	
	if (result_line == 0)
	{
		if (column == 0)
			return textForItem(entry_line, list->getRankColumn(), list, out_bold);
		else if (column == 1)
		{
			QString text = textForItem(entry_line, list->getClubColumn(), list, out_bold);
			out_bold = true;
			return text;
		}
		else if (column == 2)
		{
			QString text = textForItem(entry_line, list->getTimeColumn(), list, out_bold);
			out_bold = true;
			return text;
		}
		else //if (column == 3)
			return textForItem(entry_line, list->getPointsColumn(), list, out_bold);
	}
	else
	{
		if (column < 1)
			return "";
		else if (column == 1)
		{
			QString text = textForItem(entry_line, list->getFirstRunnerColumn() + 2*result_line - 2, list, out_bold);
			out_bold = false;
			return text;
		}
		else if (column == 2)
			return textForItem(entry_line, list->getFirstRunnerColumn() + 2*result_line - 1, list, out_bold);
		else
			return "";
	}
}
QString PresentationWidget::textForItem(int row, int column, ResultList* list, bool& out_bold)
{
	ResultList::ColumnType type = list->getColumnType(column);
	QVariant data = list->getData(row, column);
	
	if (type == ResultList::ColumnRank)
	{
		if (!data.isValid())
			return "";
		
		out_bold = true;
		int value = data.toInt();
		if (value < 0)
			return "-";
		else
			return QString::number(data.toInt()) + ".";
	}
	else if (type == ResultList::ColumnRunner)
	{
		out_bold = true;
		Runner* runner = reinterpret_cast<Runner*>(data.value<void*>());
		return runner->getFirstName() + " " + runner->getLastName();
	}
	else if (type == ResultList::ColumnPoints)
	{
		out_bold = true;
		if (data.isValid() && data.toInt() > 0)
			return pointsToString(data.toInt(), list->getDecimalPlaces(), list->getDecimalFactor());
		else
			return "";
	}
	else
	{
		out_bold = false;
		return list->getItemString(type, &data);
	}
}

void PresentationWidget::nextAnimationFrame()
{
	const float BLEND_SPEED = 2.0f;
	const float SCROLL_SPEED = 0.4f;
	
	float dt = interval / 1000.0f;
	elapsed_time += interval;
	
	bool dirty = false;
	if (current_slide_alpha < 1 && current_list)
	{
		current_slide_alpha = qMin(1.0f, current_slide_alpha + BLEND_SPEED * dt);
		dirty = true;
	}
	if (previous_list && previous_slide_alpha > 0)
	{
		previous_slide_alpha = qMax(0.0f, previous_slide_alpha - BLEND_SPEED * dt);
		if (previous_slide_alpha == 0)
			previous_list = nullptr;
		dirty = true;
	}
	if (current_scroll < current_scroll_target)
	{
		current_scroll += SCROLL_SPEED * dt;
		if (current_scroll > current_scroll_target)
			current_scroll = current_scroll_target;
		dirty = true;
		current_slide_dirty = true;
	}
	else if (current_scroll > current_scroll_target)
	{
		current_scroll -= SCROLL_SPEED * dt;
		if (current_scroll < current_scroll_target)
			current_scroll = current_scroll_target;
		dirty = true;
		current_slide_dirty = true;
	}
	
	if (dirty)
		update();
}

void PresentationWidget::showResultList(ResultList* list, int show_first_x_runners)
{
	QPixmap* temp_pixmap = previous_slide;
	
	previous_list = current_list;
	delete previous_list_img;
	previous_list_img = current_list_img;
	previous_slide = current_slide;
	previous_slide_alpha = current_slide_alpha;
	previous_scroll = current_scroll;
	
	current_list = list;
	this->show_first_x_runners = show_first_x_runners;
	current_list_img = createResultListPixmap(current_list, true);
	current_slide = temp_pixmap;
	current_slide_dirty = true;
	current_slide_alpha = 0;
	
	current_scroll = 0;
	current_scroll_target = 0;
	if (current_list_img)
	{
		Layout::Rect* list_rect = layout->getRectByID("content");
		if (list_rect)
			max_scroll = qMax(qreal(0), (current_list_img->height() - list_rect->rect.height() * display_height) / display_height);
		else
			max_scroll = 0;
	}
	else
		max_scroll = 0;
}
void PresentationWidget::setPage(int page_number, int num_pages)
{
	this->page_number = page_number;
	this->num_pages = num_pages;
	current_slide_dirty = true;
}

// ### PresentScoringController ###
PresentScoringController::PresentScoringController(Scoring* scoring, Layout* layout, QString language, ResultList* result_list, int event_year, bool show_team_scoring, bool uncover_team_scoring_gradually,
												   bool show_intermediate_team_scoring, int show_first_x_runners, bool no_runner_points, bool include_non_scoring_runners,
												   const std::vector< AbstractCategory* >& category_vector, QWidget* window)
 : scoring(scoring), layout(layout), result_list(result_list), show_team_scoring(show_team_scoring),
   show_intermediate_team_scoring(show_intermediate_team_scoring), show_first_x_runners(show_first_x_runners), category_vector(category_vector)
{
	// Create translator
	QTranslator* translator = new QTranslator();
	if (language != "en")
		translator->load("translations/" + language + ".qm");
	
	// Create pages
	std::vector< ResultList* > scoringLists;
	int flags = 0;
	if (include_non_scoring_runners)
		flags |= Scoring::IncludeAllRunners;
	scoring->calculateScoring(result_list, event_year, scoringLists, flags);
	// TODO: very bad "solution" to just take the first element of the (unnecessarily computed) result vector and assume that these are the single results (also see copy of this!)
	ResultList* single_results = scoringLists[0];
	
	int size = category_vector.size();
	pages.resize(size);
	std::map<AbstractCategory*, int> categoryToPage;
	for (int i = 0; i < size; ++i)
	{
		categoryToPage.insert(std::make_pair(category_vector[i], i));
		pages[i].type = PageType_SingleResults;
		pages[i].list = new ResultList(category_vector[i]->name, single_results->getDecimalPlaces());
		pages[i].list->addColumn(ResultList::ColumnRank);
		if (single_results->getLastRunnerColumn() == single_results->getFirstRunnerColumn())
			pages[i].list->addColumn(ResultList::ColumnRunner);
		else
		{
			for (int k = 0; k < single_results->getLastRunnerColumn() - single_results->getFirstRunnerColumn() + 1; k += 2)
			{
				pages[i].list->addColumn(ResultList::ColumnRunner);
				pages[i].list->addColumn(ResultList::ColumnTime);
			}
		}
		pages[i].list->addColumn(ResultList::ColumnClub);
		int col_time = pages[i].list->addColumn(ResultList::ColumnTime);
		pages[i].list->setTimeColumn(col_time);
		pages[i].list->addColumn(ResultList::ColumnPoints);
	}
	
	size = single_results->rowCount();
	for (int i = 0; i < size; ++i)
	{
		AbstractCategory* category = reinterpret_cast<AbstractCategory*>(single_results->getData(i, single_results->getCategoryColumn()).value<void*>());
		std::map<AbstractCategory*, int>::iterator it = categoryToPage.find(category);
		if (it == categoryToPage.end())
			continue;
		int page = it->second;
		
		QVariant rank_variant = single_results->getData(i, single_results->getRankColumn());
		if (rank_variant.isValid() && rank_variant.toInt() <= 0)
			continue;
		
		ResultList* list = pages[page].list;
		int row = list->addRow();
		list->setData(row, list->getRankColumn(), single_results->getData(i, single_results->getRankColumn()));
		if (single_results->getLastRunnerColumn() == single_results->getFirstRunnerColumn())
			list->setData(row, list->getFirstRunnerColumn(), single_results->getData(i, single_results->getFirstRunnerColumn()));
		else
		{
			for (int k = 0; k <= single_results->getLastRunnerColumn() - single_results->getFirstRunnerColumn() + 1; ++k)
				list->setData(row, list->getFirstRunnerColumn() + k, single_results->getData(i, single_results->getFirstRunnerColumn() + k));
		}
		list->setData(row, list->getClubColumn(), single_results->getData(i, single_results->getClubColumn()));
		list->setData(row, list->getTimeColumn(), single_results->getData(i, single_results->getTimeColumn()));
		list->setData(row, list->getPointsColumn(), single_results->getData(i, single_results->getPointsColumn()));
	}
	
	if (show_intermediate_team_scoring)
	{
		std::map< Club*, int > pointsPerClub;
		
		int category = 0;
		size = category_vector.size();
		for (int i = 0; i < size; ++i)
		{
			bool has_result = false;
			if (!scoring->getTeamExcludeCategories().contains(category_vector[category]->name, Qt::CaseInsensitive))
			{
				// Sum up results from this page
				ResultList* list = pages[i].list;
				int page_size = list->rowCount();
				for (int k = 0; k < page_size; ++k)
				{
					if (!list->getData(k, list->getRankColumn()).isValid())
						continue;
					has_result = true;
					
					Club* club = reinterpret_cast<Club*>(list->getData(k, list->getClubColumn()).value<void*>());
					int points = list->getData(k, list->getPointsColumn()).toInt();
					
					std::map< Club*, int >::iterator it = pointsPerClub.find(club);
					if (it == pointsPerClub.end())
						pointsPerClub.insert(std::map< Club*, int >::value_type(club, points));
					else
						it->second += points;
				}
			}
			
			// Create (intermediate) result page?
			if (i != size - 1 && (show_intermediate_team_scoring && !(pages[i].type == PageType_TeamResults)) && has_result)
			{
				PresentationPage new_page;
				new_page.type = PageType_TeamResults;
				new_page.list = new ResultList(translateWith(translator, "Intermediate team results"), single_results->getDecimalPlaces());
				int col_rank = new_page.list->addColumn(ResultList::ColumnRank);
				int col_club = new_page.list->addColumn(ResultList::ColumnClub);
				int col_points = new_page.list->addColumn(ResultList::ColumnPoints);
				
				for (std::map< Club*, int >::iterator it = pointsPerClub.begin(); it != pointsPerClub.end(); ++it)
				{
					int row = new_page.list->addRow();
					new_page.list->setData(row, col_rank, QVariant(1));
					new_page.list->setData(row, col_club, qVariantFromValue<void*>(it->first));
					new_page.list->setData(row, col_points, it->second);
				}
				
				new_page.list->calculateRanks(col_points, -1, false);
				
				pages.insert(pages.begin() + (i + 1), new_page);
				++i;
				++size;
			}
			
			++category;
		}
	}
	
	if (show_team_scoring)
	{
		// Use official results for last page
		PresentationPage new_page;
		new_page.type = PageType_TeamResults;
		new_page.list = new ResultList(*scoringLists[1], true);	// TODO: extremely bad solution to find the team results this way!
		new_page.list->setTitle(translateWith(translator, "Final team results"));
		pages.push_back(new_page);

		if (uncover_team_scoring_gradually)
		{
			int num_teams = new_page.list->rowCount();
			for (int i = num_teams - 1; i >= 1; -- i)
			{
				if (new_page.list->getRankColumn() >= 0 &&
					new_page.list->getData(i, new_page.list->getRankColumn()).toInt() == new_page.list->getData(i - 1, new_page.list->getRankColumn()).toInt())
				{
					// Uncover same ranks at the same time
					continue;
				}

				// Remove the first i teams and create a page for this
				PresentationPage new_step_page;
				new_step_page.type = PageType_TeamResults;
				new_step_page.list = new ResultList(*new_page.list, true);
				for (int k = 0; k < i; ++ k)
				{
					for (int column = 0; column < new_step_page.list->columnCount(); ++ column)
					{
						new_step_page.list->setData(k, column, QVariant());
					}
					// Ensure that we do not get "- no club -"
					if (new_step_page.list->getClubColumn() >= 0)
						new_step_page.list->setData(k, new_step_page.list->getClubColumn(), "");
				}
				pages.insert(pages.begin() + (pages.size() - 1), new_step_page);
			}
		}
	}
	
	if (no_runner_points)
	{
		// Remove points again (they are needed for intermediate results calculation)
		int size = pages.size();
		for (int i = 0; i < size; ++i)
		{
			if (pages[i].type == PageType_SingleResults)
				pages[i].list->removeColumn(pages[i].list->getPointsColumn());
		}
	}
	
	current_page = 0;
	
	delete translator;
	widget = new PresentationWidget(this, layout, QSize(1024, 768), true, window, nullptr);
}
PresentScoringController::~PresentScoringController()
{
	int size = pages.size();
	for (int i = 0; i < size; ++i)
		delete pages[i].list;
}

QString PresentScoringController::translateWith(QTranslator* translator, const char* string)
{
	if (translator->isEmpty())
		return string;

	QString result = translator->translate("presentation", string);
	if (result.isEmpty())
		return string;
	else
		return result;
}

void PresentScoringController::setPage(int page)
{
	if (page >= (int)pages.size())
		page = pages.size() - 1;
	else if (page < 0)
		page = 0;
	
	if (page == current_page)
		return;
	
	current_page = page;
	widget->showResultList(pages[current_page].list, (pages[current_page].type == PageType_SingleResults) ? show_first_x_runners : -1);
	widget->setPage(current_page + 1, pages.size());
}

void PresentScoringController::start()
{
	int temp = current_page;
	current_page = -1;
	setPage(temp);
}
void PresentScoringController::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
		setPage(current_page + 1);
}
bool PresentScoringController::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Right || event->key() == Qt::Key_Down)
	{
		setPage(current_page + 1);
		return true;
	}
	else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Up)
	{
		setPage(current_page - 1);
		return true;
	}
	else if (event->key() == Qt::Key_Home)
	{
		setPage(0);
		return true;
	}
	else if (event->key() == Qt::Key_End)
	{
		setPage(pages.size() - 1);
		return true;
	}
	return false;
}

// ### PresentScoringDialog ###

class QListWidgetDragDropFix : public QListWidget
{
public:
	void dropEvent(QDropEvent* event)
	{
		if (event->proposedAction() != Qt::MoveAction)
			return;
		QListView::dropEvent(event);
	}
};

PresentScoringDialog::PresentScoringDialog(Event* event, int event_year, ResultList* result_list, QWidget* parent): QDialog(parent), event(event), event_year(event_year), result_list(result_list)
{
	setWindowTitle(tr("Present scoring"));
	
	QLabel* scoringLabel = new QLabel(tr("Scoring:"));
	scoringCombo = new QComboBox();
	
	teamScoringCheck = new QCheckBox(tr("Show team scoring"));
	teamScoringStepByStepCheck = new QCheckBox(tr("Uncover team scoring one-by-one"));
	teamScoringCategoryCheck = new QCheckBox(tr("Show intermediate team scoring after each category"));
	showOnlyFirstXCheck = new QCheckBox(tr("Only show up to first"));
	showOnlyFirstXEdit = new QLineEdit(tr("3"));
	showOnlyFirstXEdit->setValidator(new QIntValidator(1, 999999, showOnlyFirstXEdit));
	QLabel* showOnlyFirstXLabel = new QLabel(tr("results"));
	doNotShowRunnerPointsCheck = new QCheckBox(tr("Do not show points for runners"));
	includeNonScoringRunnersCheck = new QCheckBox(tr("Include runners which are not in this scoring"));
	
	QLabel* categoriesLabel = new QLabel(tr("Drag the categories with the mouse from one list to the other, or to reorder them.\nThey will be presented in top-down order."));
	QLabel* presentCategoriesLabel = new QLabel(tr("Present these categories:"));
	QLabel* otherCategoriesLabel = new QLabel(tr("Do not show these:"));
	categoryList = new QListWidgetDragDropFix();
	otherCategoryList = new QListWidgetDragDropFix();
	
	QLabel* layoutLabel = new QLabel(tr("Layout:"));
	layoutCombo = new QComboBox();
	
	QLabel* languageLabel = new QLabel(tr("Language:"));
	languageCombo = new QComboBox();

	categoryList->setDragEnabled(true);
	categoryList->setDefaultDropAction(Qt::MoveAction);
	categoryList->setDragDropMode(QAbstractItemView::DragDrop);
	categoryList->viewport()->setAcceptDrops(true);
	categoryList->setDropIndicatorShown(true);
	otherCategoryList->setDragEnabled(true);
	otherCategoryList->setDefaultDropAction(Qt::MoveAction);
	otherCategoryList->setDragDropMode(QAbstractItemView::DragDrop);
	otherCategoryList->viewport()->setAcceptDrops(true);
	otherCategoryList->setDropIndicatorShown(true);
	
	QPushButton* backButton = new QPushButton(tr("Back"));
	QPushButton* presentButton = new QPushButton(QIcon("images/display.png"), tr("Start"));
	
	QHBoxLayout* scoringLayout = new QHBoxLayout();
	scoringLayout->addWidget(scoringLabel);
	scoringLayout->addWidget(scoringCombo);
	scoringLayout->addStretch(1);
	
	QHBoxLayout* showOnlyFirstXLayout = new QHBoxLayout();
	showOnlyFirstXLayout->addWidget(showOnlyFirstXCheck);
	showOnlyFirstXLayout->addWidget(showOnlyFirstXEdit);
	showOnlyFirstXLayout->addWidget(showOnlyFirstXLabel);
	showOnlyFirstXLayout->addStretch(1);
	
	QGridLayout* categoriesLayout = new QGridLayout();
	categoriesLayout->addWidget(categoriesLabel, 0, 0, 1, 2);
	categoriesLayout->addWidget(presentCategoriesLabel, 1, 0);
	categoriesLayout->addWidget(otherCategoriesLabel, 1, 1);
	categoriesLayout->addWidget(categoryList, 2, 0);
	categoriesLayout->addWidget(otherCategoryList, 2, 1);
	
	QHBoxLayout* layoutLayout = new QHBoxLayout();
	layoutLayout->addWidget(layoutLabel);
	layoutLayout->addWidget(layoutCombo);
	layoutLayout->addStretch(1);
	
	QHBoxLayout* languageLayout = new QHBoxLayout();
	languageLayout->addWidget(languageLabel);
	languageLayout->addWidget(languageCombo);
	languageLayout->addStretch(1);
	
	QHBoxLayout* buttonsLayout = new QHBoxLayout();
	buttonsLayout->addWidget(backButton);
	buttonsLayout->addStretch(1);
	buttonsLayout->addWidget(presentButton);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(scoringLayout);
	layout->addWidget(teamScoringCheck);
	layout->addWidget(teamScoringStepByStepCheck);
	layout->addWidget(teamScoringCategoryCheck);
	layout->addLayout(showOnlyFirstXLayout);
	layout->addWidget(doNotShowRunnerPointsCheck);
	layout->addWidget(includeNonScoringRunnersCheck);
	layout->addSpacing(16);
	layout->addLayout(categoriesLayout);
	layout->addSpacing(16);
	layout->addLayout(layoutLayout);
	layout->addLayout(languageLayout);
	layout->addSpacing(16);
	layout->addLayout(buttonsLayout);
	setLayout(layout);
	
	connect(scoringCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(currentScoringChanged(int)));
	
	// Set values
	int num_scorings = event->getNumScorings();
	for (int i = 0; i < num_scorings; ++i)
	{
		Scoring* scoring = event->getScoring(i);
		scoringCombo->addItem(scoring->getFileName(), qVariantFromValue<void*>(scoring));
	}
	
	for (LayoutDB::Layouts::iterator it = layoutDB.begin(); it != layoutDB.end(); ++it)
		layoutCombo->addItem(it.key(), qVariantFromValue<void*>(it.value()));
	
	showOnlyFirstXEdit->setEnabled(false);
	
	QStringList myLanguages;
	QDir dir("translations");
	if (dir.exists())
	{
		QStringList nameFilters;
		nameFilters << ("*.qm");
		myLanguages = dir.entryList(nameFilters, QDir::Files | QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::Name);
		for (int i = 0; i < myLanguages.size(); ++i)
			myLanguages[i] = myLanguages[i].left(myLanguages[i].size() - 3);
	}
	myLanguages.insert(0, "en");
	languageCombo->addItems(myLanguages);
	int locale_language = languageCombo->findText(QLocale::system().languageToString(QLocale::system().language()), Qt::MatchFixedString | Qt::MatchStartsWith);
	if (locale_language != -1)
		languageCombo->setCurrentIndex(locale_language);
	
	connect(teamScoringCheck, SIGNAL(clicked(bool)), this, SLOT(teamScoringChecked(bool)));
	connect(showOnlyFirstXCheck, SIGNAL(clicked(bool)), this, SLOT(showOnlyFirstXChecked(bool)));
	connect(includeNonScoringRunnersCheck, SIGNAL(clicked(bool)), this, SLOT(includeNonScoringRunnersClicked(bool)));
	connect(backButton, SIGNAL(clicked(bool)), this, SLOT(close()));
	connect(presentButton, SIGNAL(clicked(bool)), this, SLOT(presentClicked()));
}
PresentScoringDialog::~PresentScoringDialog()
{
}

void PresentScoringDialog::teamScoringChecked(bool checked)
{
	teamScoringStepByStepCheck->setEnabled(checked);
	teamScoringCategoryCheck->setEnabled(checked);
}
void PresentScoringDialog::showOnlyFirstXChecked(bool checked)
{
	showOnlyFirstXEdit->setEnabled(checked);
}
void PresentScoringDialog::currentScoringChanged(int index)
{
	assert(index != -1);
	Scoring* scoring = reinterpret_cast<Scoring*>(scoringCombo->itemData(index, Qt::UserRole).value<void*>());
	
	bool enable = scoring->getTeamScoring();
	teamScoringCheck->setEnabled(enable);
	teamScoringStepByStepCheck->setEnabled(teamScoringCheck->isChecked() && enable);
	teamScoringCategoryCheck->setEnabled(teamScoringCheck->isChecked() && enable);
	
	recalculateCategories();
}
void PresentScoringDialog::includeNonScoringRunnersClicked(bool checked)
{
	recalculateCategories();
}

void PresentScoringDialog::recalculateCategories()
{
	int index = scoringCombo->currentIndex();
	if (index < 0)
		return;
	Scoring* scoring = reinterpret_cast<Scoring*>(scoringCombo->itemData(index, Qt::UserRole).value<void*>());
	
	int flags = 0;
	if (includeNonScoringRunnersCheck->isChecked())
		flags |= Scoring::IncludeAllRunners;
	std::vector< ResultList* > scoringLists;
	scoring->calculateScoring(result_list, event_year, scoringLists, flags);
	// TODO: very bad "solution" to just take the first element of the (unnecessarily computed) result vector and assume that these are the single results (also see copy of this!)
	ResultList* single_results = scoringLists[0];
	
	category_map.clear();
	categoryList->clear();
	otherCategoryList->clear();
	int num_rows = single_results->rowCount();
	for (int i = 0; i < num_rows; ++i)
	{
		AbstractCategory* category = reinterpret_cast<AbstractCategory*>(single_results->getData(i, single_results->getCategoryColumn()).value<void*>());
		
		if (category_map.insert(std::make_pair(category->number, category)).second == true)
		{
			QListWidgetItem* new_item = new QListWidgetItem(category->name);
			new_item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
			new_item->setData(Qt::UserRole, category->number);
			categoryList->addItem(new_item);
		}
	}
	
	for (int i = 0; i < (int)scoringLists.size(); ++i)
		delete scoringLists[i];
}

void PresentScoringDialog::presentClicked()
{
	if (categoryList->count() == 0)
	{
		QMessageBox::warning(this, APP_NAME, tr("There are no categories to be shown. Make sure to drag some categories into the left list!"));
		return;
	}
	
	Layout* layout = reinterpret_cast<Layout*>(layoutCombo->itemData(layoutCombo->currentIndex(), Qt::UserRole).value<void*>());
	if (!layout)
	{
		layout = layoutDB.getOrLoadLayout(layoutCombo->itemText(layoutCombo->currentIndex()), this);
		if (!layout)
		{
			QMessageBox::warning(this, APP_NAME, tr("Error loading the layout, cannot start presentation!"));
			return;
		}
		layoutCombo->setItemData(layoutCombo->currentIndex(), qVariantFromValue<void*>(layout), Qt::UserRole);
	}
	
	std::vector<AbstractCategory*> category_vector;
	for (int i = 0; i < categoryList->count(); ++i)
		category_vector.push_back(category_map[categoryList->item(i)->data(Qt::UserRole).toInt()]);
	
	int show_first_x_runners = -1;
	if (showOnlyFirstXCheck->isChecked())
		show_first_x_runners = showOnlyFirstXEdit->text().toInt();
	QString language = languageCombo->itemText(languageCombo->currentIndex());
	
	Scoring* scoring = reinterpret_cast<Scoring*>(scoringCombo->itemData(scoringCombo->currentIndex(), Qt::UserRole).value<void*>());
	PresentScoringController* controller = new PresentScoringController(scoring, layout, language, result_list, event_year,
																		teamScoringCheck->isEnabled() && teamScoringCheck->isChecked(),
																		teamScoringStepByStepCheck->isEnabled() && teamScoringStepByStepCheck->isChecked(),
																		teamScoringCategoryCheck->isEnabled() && teamScoringCategoryCheck->isChecked(),
																		show_first_x_runners,
																		doNotShowRunnerPointsCheck->isEnabled() && doNotShowRunnerPointsCheck->isChecked(),
																		includeNonScoringRunnersCheck->isEnabled() && includeNonScoringRunnersCheck->isChecked(),
																		category_vector, this);
	controller->start();
}
