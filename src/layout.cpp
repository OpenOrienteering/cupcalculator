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


#include "layout.h"

#include <QDir>
#include <QFile>
#include <QXmlStreamReader>
#include <QMessageBox>
#include <QPainter>

Layout::Layout(const QString& filename) : filename(filename)
{
	// set defaults
	has_background_color = false;
	background_color = Qt::white;
	background_image = NULL;
	background_image_cache = NULL;
	owns_background_image = false;
	aspect = 4 / 3.0f;
}
Layout::Layout(const Layout& other)
{
	aspect = other.aspect;
	has_background_color = other.has_background_color;
	background_color = other.background_color;
	background_image = other.background_image;
	background_image_cache = NULL;
	owns_background_image = false;
	
	for (std::map<QString, Rect*>::const_iterator it = other.rects.begin(); it != other.rects.end(); ++it)
		rects.insert(std::make_pair(it->first, new Rect(*it->second)));
	for (std::map<QString, Point*>::const_iterator it = other.points.begin(); it != other.points.end(); ++it)
		points.insert(std::make_pair(it->first, new Point(*it->second)));
	//for (std::map<QString, Font*>::const_iterator it = other.fonts.begin(); it != other.fonts.end(); ++it)
	//	fonts.insert(std::make_pair(it->first, new Font(*it->second)));
	
	filename = other.filename;
}
Layout::~Layout()
{
	for (std::map<QString, Rect*>::iterator it = rects.begin(); it != rects.end(); ++it)
		delete it->second;
	for (std::map<QString, Point*>::iterator it = points.begin(); it != points.end(); ++it)
		delete it->second;
	for (std::map<QString, Font*>::iterator it = fonts.begin(); it != fonts.end(); ++it)
		delete it->second;
	
	if (owns_background_image)
		delete background_image;
	delete background_image_cache;
}

Layout::Point* Layout::getPointByID(const QString& id)
{
	std::map<QString, Point*>::iterator it = points.find(id);
	return (it == points.end()) ? NULL : it->second;
}
Layout::Rect* Layout::getRectByID(const QString& id)
{
	std::map<QString, Rect*>::iterator it = rects.find(id);
	return (it == rects.end()) ? NULL : it->second;
}

QFont Layout::qFontFromFont(Layout::Font* font_data, int display_height)
{
	QFont font;
	font.setFamily(font_data->family);
	font.setPixelSize((int)(font_data->size * display_height + 0.5f));
	font.setWeight(font_data->weight);
	return font;
}

void Layout::draw(QPainter* painter, float width, float height)
{
	if (hasBackgroundColor())
		painter->fillRect(0, 0, width, height, getBackgroundColor());
	if (background_image)
	{
		if (width == background_image->width() && height == background_image->height())
			painter->drawPixmap(QRectF(0, 0, width, height), *background_image, QRectF(0, 0, background_image->width(), background_image->height()));
		else
		{
			if (!background_image_cache || background_image_cache->width() != width || background_image_cache->height() != height)
			{
				delete background_image_cache;
				background_image_cache = new QPixmap(width, height);
				
				QPainter cache_painter(background_image_cache);
				cache_painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
				cache_painter.drawPixmap(QRectF(0, 0, width, height), *background_image, QRectF(0, 0, background_image->width(), background_image->height()));
				cache_painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
			}
		
			painter->drawPixmap(QRectF(0, 0, width, height), *background_image_cache, QRectF(0, 0, background_image_cache->width(), background_image_cache->height()));
		}
	}
	
	for (std::map<QString, Point*>::iterator it = points.begin(); it != points.end(); ++it)
	{
		Point* point = it->second;
		
		Font* font_data = point->strong_font ? point->font_strong : point->font_normal;
		QFont font = qFontFromFont(font_data, height);
		QFontMetricsF font_metrics(font);
		
		// NOTE: Could optimize this by caching the size
		float text_width = font_metrics.width(point->text);
		float text_height = font_metrics.height();	// TODO: Count lines
		
		QRectF rect = QRectF(point->pos.x()*width - 0.5f*text_width, point->pos.y()*height - 0.5f*text_height, text_width, text_height);
		Qt::Alignment alignment = point->alignment;
		if (point->alignment & Qt::AlignLeft)
			rect.translate(0.5f*text_width, 0);
		else if (point->alignment & Qt::AlignRight)
			rect.translate(-0.5f*text_width, 0);
		if (point->alignment & Qt::AlignTop)
			rect.translate(0, 0.5f*text_height);
		else if (point->alignment & Qt::AlignBottom)
			rect.translate(0, -0.5f*text_height);
		else if (point->alignment & Qt::AlignAbsolute)
		{
			rect.translate(0, 0.5f*text_height - font_metrics.ascent());
			alignment = (alignment & ~Qt::AlignAbsolute) | Qt::AlignVCenter;
		}
		
		painter->setFont(font);
		painter->setPen(font_data->color);
		
		if (point->max_width > 0)
			painter->drawText(rect, alignment, font_metrics.elidedText(point->text, Qt::ElideMiddle, point->max_width*width));
		else
			painter->drawText(rect, alignment, point->text);
	}
}

bool Layout::loadFromFile()
{
	QFile file("my layouts/" + filename + ".xml");
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::warning(NULL, tr("Error"), tr("Could not load layout: %1 because the file could not be opened!").arg(filename));
		return false;
	}
	
	QXmlStreamReader stream(&file);
	while (!stream.atEnd())
	{
		stream.readNext();
		if (stream.tokenType() != QXmlStreamReader::StartElement)
			continue;
		
		if (stream.name() == "Layout")
		{
			float width = stream.attributes().value("aspectWidth").toString().toInt();
			float height = stream.attributes().value("aspectHeight").toString().toInt();
			aspect = width / height;
		}
		else if (stream.name() == "Background")
		{
			QStringRef ref = stream.attributes().value("color");
			if (!ref.isEmpty())
			{
				has_background_color = true;
				background_color = QColor(ref.toString());
			}
			
			ref = stream.attributes().value("image");
			if (!ref.isEmpty())
			{
				background_image = new QPixmap("my layouts/" + ref.toString());
				owns_background_image = true;
			}
		}
		else if (stream.name() == "Font")
		{
			Font* new_font = new Font;
			new_font->family = stream.attributes().value("family").toString();
			new_font->size = stream.attributes().value("size").toString().toFloat();
			QStringRef ref = stream.attributes().value("lineSpacing");
			if (ref.isEmpty())
				new_font->line_spacing = 1;
			else
				new_font->line_spacing = ref.toString().toFloat();
			ref = stream.attributes().value("weight");
			if (ref.isEmpty())
				new_font->weight = QFont::Normal;
			else
			{
				if (ref == "normal")
					new_font->weight = QFont::Normal;
				else if (ref == "bold")
					new_font->weight = QFont::Bold;
				else if (ref == "black")
					new_font->weight = QFont::Black;
				else
				{
					// TODO: Error message
					delete new_font;
					return false;
				}
			}
			new_font->color = QColor(stream.attributes().value("color").toString());
			
			fonts.insert(std::make_pair(stream.attributes().value("id").toString(), new_font));
		}
		else if (stream.name() == "Rect")
		{
			Rect* new_rect = new Rect();
			new_rect->rect = QRectF(QPointF(stream.attributes().value("left").toString().toFloat(), stream.attributes().value("top").toString().toFloat()),
									QPointF(stream.attributes().value("right").toString().toFloat(), stream.attributes().value("bottom").toString().toFloat()));
			getFonts(stream.attributes(), new_rect->font_normal, new_rect->font_strong);
			
			rects.insert(std::make_pair(stream.attributes().value("id").toString(), new_rect));
		}
		else if (stream.name() == "Point")
		{
			Point* new_point = new Point();
			new_point->pos = QPointF(stream.attributes().value("x").toString().toFloat(), stream.attributes().value("y").toString().toFloat());
			getFonts(stream.attributes(), new_point->font_normal, new_point->font_strong);
			
			QStringRef ref = stream.attributes().value("max_width");
			if (ref.isEmpty())
				new_point->max_width = -1;
			else
				new_point->max_width = ref.toString().toFloat();
			
			Qt::Alignment align_h = Qt::AlignHCenter;
			QString temp = stream.attributes().value("align_h").toString();
			if (temp == "left")
				align_h = Qt::AlignLeft;
			else if (temp == "right")
				align_h = Qt::AlignRight;
			
			Qt::Alignment align_v = Qt::AlignVCenter;
			temp = stream.attributes().value("align_v").toString();
			if (temp == "bottom")
				align_v = Qt::AlignBottom;
			else if (temp == "top")
				align_v = Qt::AlignTop;
			else if (temp == "baseline")
				align_v = Qt::AlignAbsolute;	// NOTE: This is being misused as there seems to be no "baseline" flag!
			
			new_point->strong_font = false;
			new_point->alignment = align_h | align_v;
			
			ref = stream.attributes().value("text");
			if (ref.isEmpty())
				new_point->text = "";
			else
				new_point->text = ref.toString();
			
			points.insert(std::make_pair(stream.attributes().value("id").toString(), new_point));
		}
	}
	return true;
}
void Layout::getFonts(const QXmlStreamAttributes& attributes, Layout::Font*& font_normal, Layout::Font*& font_strong)
{
	QStringRef ref = attributes.value("font");
	if (!ref.isEmpty())
	{
		font_normal = getFont(ref.toString());
		font_strong = font_normal;
	}
	
	ref = attributes.value("font_normal");
	if (!ref.isEmpty())
		font_normal = getFont(ref.toString());
	
	ref = attributes.value("font_strong");
	if (!ref.isEmpty())
		font_strong = getFont(ref.toString());
	
	//assert(font_normal && font_strong);
}
Layout::Font* Layout::getFont(const QString& name)
{
	std::map<QString, Font*>::iterator it = fonts.find(name);
	if (it == fonts.end())
	{
		QMessageBox::warning(NULL, tr("Error while loading layout"), tr("Cannot find font %1!").arg(name));
		return NULL;
	}
	
	return it->second;
}

// ### LayoutDB ###

LayoutDB::LayoutDB()
{
	const QString dir_name = "my layouts";
	
	QDir dir(dir_name);
	if (!dir.exists())
	{
		QDir curDir;
		curDir.mkdir(dir_name);
		dir = QDir(dir_name);
	}
	QStringList nameFilters;
	nameFilters << ("*.xml");
	QStringList myLayouts = dir.entryList(nameFilters, QDir::Files | QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDir::Name);
	
	for (int i = 0; i < myLayouts.size(); ++i)
		layouts.insert(myLayouts.at(i).left(myLayouts.at(i).size() - 4), NULL);
	
	// TODO: dir watcher
}
Layout* LayoutDB::getOrLoadLayout(const QString& name, QWidget* dialogParent)
{
	Layouts::iterator it = layouts.find(name);
	if (it == layouts.end())
		return NULL;
	
	if (*it)
		return *it;

	// Scoring not loaded yet
	Layout* loadedLayout = new Layout(name);
	if (!loadedLayout->loadFromFile())
	{
		delete loadedLayout;
		return NULL;
	}
	layouts.insert(name, loadedLayout);
	
	return loadedLayout;
}
