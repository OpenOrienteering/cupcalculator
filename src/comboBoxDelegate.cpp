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


#include "comboBoxDelegate.h"

#include <QComboBox>

ComboBoxDelegate::ComboBoxDelegate(QAbstractItemModel* _model, int _model_column, QObject* parent)
	: QItemDelegate(parent), model(_model), model_column(_model_column)
{
}

QWidget* ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QComboBox* combo = new QComboBox(parent);

	combo->setModel(model);
	combo->setModelColumn(model_column);

	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(emitCommitData()));
	return combo;
}

void ComboBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	QComboBox* combo = static_cast<QComboBox*>(editor);
	QString value = index.model()->data(index, Qt::EditRole).toString();

	int pos = combo->findText(value, Qt::MatchExactly);
	combo->setCurrentIndex(pos);
}

void ComboBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	QComboBox* combo = static_cast<QComboBox*>(editor);
	QString value = combo->currentText();
	
	model->setData(index, value, Qt::EditRole);
}

void ComboBoxDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	editor->setGeometry(option.rect);
}

void ComboBoxDelegate::emitCommitData()
{
	emit commitData(qobject_cast<QWidget*>(sender()));
}

#include "comboBoxDelegate.moc"
