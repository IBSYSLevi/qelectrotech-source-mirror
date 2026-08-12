/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cabinetlayoutsourcemodel.h"

#include "../projectdatabase.h"
#include "../../qetproject.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMimeData>
#include <QFont>
#include <QBrush>

const QString CabinetLayoutSourceModel::CABINET_LAYOUT_SOURCE_MIME_TYPE =
		QStringLiteral("application/x-qet-cabinet-layout-uuid");

/**
	@brief CabinetLayoutSourceModel::CabinetLayoutSourceModel
	@param project the project whose placed elements are listed.
	@param parent
*/
CabinetLayoutSourceModel::CabinetLayoutSourceModel(QETProject *project, QObject *parent) :
	QStandardItemModel(parent),
	m_project(project)
{
	setColumnCount(1);
	setHorizontalHeaderLabels({tr("Éléments du projet")});

	if (m_project && m_project->dataBase()) {
		connect(m_project->dataBase(), &projectDataBase::dataBaseUpdated,
				this, &CabinetLayoutSourceModel::reload);
	}

	reload();
}

/**
	@brief CabinetLayoutSourceModel::reload
	Rebuild the whole tree from the project's element_nomenclature_view.
	Folios are the top-level items (one per diagram, ordered by folio
	number where numeric), elements placed on that folio are their
	children. Elements with neither a width nor a height set are
	skipped, since they cannot yield a meaningful cabinet layout box.
*/
void CabinetLayoutSourceModel::reload()
{
	removeRows(0, rowCount());

	if (!m_project || !m_project->dataBase())
		return;

	QSqlQuery query = m_project->dataBase()->newQuery(
		"SELECT title, folio, element_uuid, label, function, "
		"		description, manufacturer, "
		"       width, height, depth "
		"FROM element_nomenclature_view "
		"ORDER BY CAST(folio AS INTEGER), folio, label"
	);

	if (!query.exec()) {
		qWarning() << "CabinetLayoutSourceModel::reload SQL error:"
				   << query.lastError();
		return;
	}

	QStandardItem *root = invisibleRootItem();
	QStandardItem *current_folio_item = nullptr;
	QString current_folio_key;

	while (query.next())
	{
		const QString title		= query.value("title").toString();
		const QString folio		= query.value("folio").toString();
		const QString uuid		= query.value("element_uuid").toString();
		const QString label		= query.value("label").toString();
		const QString func		= query.value("function").toString();
		const QString desc		= query.value("description").toString();
		const QString mfr		= query.value("manufacturer").toString();
		const QString width		= query.value("width").toString();
		const QString height	= query.value("height").toString();
		const QString depth		= query.value("depth").toString();

		const bool has_dimensions = !width.isEmpty() || !height.isEmpty();

		const QString folio_key = folio + '\x1f' + title;
		if (folio_key != current_folio_key)
		{
			const QString folio_label = folio.isEmpty()
					? title
					: tr("%1 — %2").arg(folio, title);
			current_folio_item = new QStandardItem(folio_label);
			current_folio_item->setEditable(false);
			current_folio_item->setSelectable(false);
			current_folio_item->setDragEnabled(false);
			root->appendRow(current_folio_item);
			current_folio_key = folio_key;
		}

		QString element_label = label.isEmpty() ? tr("(element sans label)") : label;
		if (!func.isEmpty())
			element_label += QStringLiteral(" — ") + func;
		if (!desc.isEmpty())
			element_label += QStringLiteral(" — ") + desc;
		if (!mfr.isEmpty())
			element_label += QStringLiteral(" — ") + mfr;
		if (!has_dimensions)
			element_label = QStringLiteral("⚠ ") + element_label;

		auto *element_item = new QStandardItem(element_label);
		element_item->setEditable(false);
		element_item->setDragEnabled(has_dimensions);
		element_item->setDropEnabled(false);
		element_item->setData(uuid, Qt::UserRole);
		element_item->setData(width,  Qt::UserRole + 1);
		element_item->setData(height, Qt::UserRole + 2);
		element_item->setData(depth,  Qt::UserRole + 3);

		if (has_dimensions) {
			element_item->setToolTip(
				tr("Largeur : %1 mm, Hauteur : %2 mm, Profondeur : %3 mm")
					.arg(width.isEmpty()  ? QStringLiteral("—") : width)
					.arg(height.isEmpty() ? QStringLiteral("—") : height)
					.arg(depth.isEmpty()  ? QStringLiteral("—") : depth));
		} else {
			QFont font = element_item->font();
			font.setItalic(true);
			element_item->setFont(font);
			element_item->setForeground(QBrush(Qt::gray));
			element_item->setToolTip(
				tr("Aucune dimension renseignée pour cet élément -- "
				   "ajoutez au moins la largeur ou la hauteur dans "
				   "l'éditeur d'élément pour pouvoir le glisser ici."));
		}

		if (current_folio_item)
			current_folio_item->appendRow(element_item);
	}
}

QStringList CabinetLayoutSourceModel::mimeTypes() const
{
	return {CABINET_LAYOUT_SOURCE_MIME_TYPE};
}

/**
	@brief CabinetLayoutSourceModel::mimeData
	Only the element's UUID is carried in the drag payload -- width,
	height and label are looked up fresh from the database at drop
	time, the same way ElementsCollectionModel only carries a
	collection path rather than the symbol's full geometry. This
	avoids placing a cabinet layout box with stale dimensions if the
	source element was edited after the drag started.
*/
QMimeData *CabinetLayoutSourceModel::mimeData(const QModelIndexList &indexes) const
{
	if (indexes.isEmpty())
		return nullptr;

	const QModelIndex index = indexes.first();
	const QString uuid = index.data(Qt::UserRole).toString();
	if (uuid.isEmpty())
		return nullptr;

	auto *mime_data = new QMimeData();
	mime_data->setData(CABINET_LAYOUT_SOURCE_MIME_TYPE, uuid.toLatin1());
	return mime_data;
}