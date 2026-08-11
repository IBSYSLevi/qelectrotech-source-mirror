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

#ifndef CABINETLAYOUTSOURCEMODEL_H
#define CABINETLAYOUTSOURCEMODEL_H

#include <QStandardItemModel>
#include <QPointer>

class QETProject;

/**
	@brief The CabinetLayoutSourceModel class
	Populates a tree of the elements currently placed in a project,
	grouped by the folio (diagram) they are placed on, for use as a
	drag source when building a scale-accurate cabinet/panel layout
	drawing ("disposition des armoires").

	Top-level items represent folios (one per diagram, labelled with
	its title and folio number). Their children represent the elements
	placed on that folio that are eligible for a cabinet layout box
	(i.e. not excluded from the BOM and carrying at least a width or
	height).

	Each element item carries, via Qt::UserRole, the source element's
	UUID -- used at drop time to look up the current width/height/depth
	and label directly from the project database, rather than caching
	values that could go stale.

	The model rebuilds itself whenever the project's database signals
	that it was updated (@see projectDataBase::dataBaseUpdated), so it
	stays in sync as elements are added, removed or edited.
*/
class CabinetLayoutSourceModel : public QStandardItemModel
{
	Q_OBJECT

	public:
		explicit CabinetLayoutSourceModel(QETProject *project, QObject *parent = nullptr);
		~CabinetLayoutSourceModel() override = default;

		QStringList mimeTypes() const override;
		QMimeData *mimeData(const QModelIndexList &indexes) const override;

		static const QString CABINET_LAYOUT_SOURCE_MIME_TYPE;

	public slots:
		void reload();

	private:
		QPointer<QETProject> m_project;
};

#endif // CABINETLAYOUTSOURCEMODEL_H