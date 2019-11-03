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


#ifndef CUPCALCULATOR_LAYOUT_H
#define CUPCALCULATOR_LAYOUT_H

#include <vector>
#include <map>

#include <QMap>
#include <QString>
#include <QRect>
#include <QColor>
#include <QFont>
#include <QXmlStreamAttributes>

class Layout : public QObject
{
Q_OBJECT
public:
	
	struct Font
	{
		QString family;
		float size;
		int weight;
		float line_spacing;
		QColor color;
	};
	
	struct Rect
	{
		QRectF rect;
		Font* font_normal;
		Font* font_strong;
	};
	
	struct Point
	{
		QPointF pos;
		QString text;
		float max_width;
		Qt::Alignment alignment;
		bool strong_font;
		Font* font_normal;
		Font* font_strong;
	};
	
	Layout(const QString& filename);
	/// Creates a copy of the layout. NOTE: The font pointers will still point to the original layout, though that should be no problem as nobody wants to change fonts in a copyied layout ...
	Layout(const Layout& other);
	~Layout();
	
	bool loadFromFile();
	
	Point* getPointByID(const QString& id);
	Rect* getRectByID(const QString& id);
	
	QFont qFontFromFont(Layout::Font* font_data, int display_height);
	
	inline bool hasBackgroundColor() const {return has_background_color;}
	inline QColor getBackgroundColor() const {return background_color;}
    void draw(QPainter* painter, float width, float height);
	
private:
	
	void getFonts(const QXmlStreamAttributes& attributes, Font*& font_normal, Font*& font_strong);
	Font* getFont(const QString& name);
	
	float aspect;	// width / height
	bool has_background_color;
	QColor background_color;
	QPixmap* background_image;
	QPixmap* background_image_cache;
	bool owns_background_image;	// must the image be deleted in the destructor?
	
	std::map<QString, Font*> fonts;		// name -> font
	std::map<QString, Rect*> rects;		// id -> rect
	std::map<QString, Point*> points;	// id -> point
	
	QString filename;
};

/// Manages all layouts which were ever used, loads only those that are necessary
class LayoutDB
{
public:
	
	typedef QMap<QString, Layout*> Layouts;
	
	/// Returns the layout with the given name; loads it if it is not loaded yet.
	/// Returns nullptr if there was an error loading it.
	Layout* getOrLoadLayout(const QString& name, QWidget* dialogParent);
	
	inline Layouts::iterator begin() {return layouts.begin();}
	inline Layouts::iterator end() {return layouts.end();}
	inline bool contains(const QString& name) {return layouts.contains(name);}
	inline int size() {return layouts.size();}
	
	static inline LayoutDB& getSingleton()
	{
		static LayoutDB instance;
		return instance;
	}
	
private:
	
	LayoutDB();
	
	/// The value can be nullptr if the layout is not loaded yet
	Layouts layouts;
};

#define layoutDB LayoutDB::getSingleton()

#endif
