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

#ifndef CABINETLAYOUTREFERENCEITEM_H
#define CABINETLAYOUTREFERENCEITEM_H

#include "qetgraphicsitem.h"

#include <QUuid>
#include <QFont>

class QETProject;

/**
	@brief The CabinetLayoutReferenceItem class
	A scale-accurate box representing, on a cabinet layout
	("disposition des armoires") folio, the real-world footprint of a
	device placed elsewhere in the project -- front view (width x
	height) or side view (depth x height), depending on the folio's
	own Diagram::cabinetLayoutView() setting.

	Unlike the earlier CabinetLayoutBoxFactory/Element-based approach,
	this is a dedicated, lightweight top-level diagram item type (see
	Diagram::toXml()/fromXml(), which already dispatches over several
	such types -- Conductor, IndependentTextItem, QetShapeItem, etc. --
	via item->type()), not a library symbol:
	- No .elmt file, no ElementsLocation, no project-embedded
	  collection entry is involved in creating one.
	- No auto-numbering/formula machinery -- the label always comes
	  directly from the referenced source element, never
	  auto-generated.
	- Not tied to Element's fixed, author-time symbol geometry: its
	  size is entirely computed from the source device's
	  width/height/depth and the folio's scale.

	The item stores only what can't be recomputed: the source
	element's UUID, its own position and rotation on this folio, and
	which view (front/side) it represents. Everything else --
	label, current dimensions -- is looked up fresh from the project
	database each time it's needed (@see refreshFromSource), the same
	"don't cache what you can look up" principle already used by
	CabinetLayoutSourceModel.
*/

class CabinetLayoutReferenceItem : public QetGraphicsItem
{
	Q_OBJECT

	public:
		explicit CabinetLayoutReferenceItem(const QUuid &source_element_uuid,
											 bool is_side_view,
											 QGraphicsItem *parent = nullptr);
		~CabinetLayoutReferenceItem() override;

		enum { Type = UserType + 1012 };
		int type() const override { return Type; }

		QRectF boundingRect() const override;
		void paint(QPainter *painter,
				   const QStyleOptionGraphicsItem *options,
				   QWidget *widget = nullptr) override;

		QDomElement toXml(QDomDocument &document) const;
		bool fromXml(const QDomElement &dom_element);

		QUuid sourceElementUuid() const { return m_source_element_uuid; }
		bool isSideView() const { return m_is_side_view; }

		/**
			@brief refreshFromSource
			Re-reads the current label/width/height/depth for
			sourceElementUuid() from the project database and
			recomputes this item's on-screen size accordingly.
			Called once right after construction, and again whenever
			the source element's own data changes
			(@see CabinetLayoutReferenceWatcher).
			@return false if the source element could no longer be
			found in the project database (e.g. it was deleted) --
			callers should then remove this item rather than leave a
			reference pointing at nothing.
		*/
		bool refreshFromSource();

		/**
			@brief jumpToSource
			Selects sourceElementUuid()'s element on its own folio and
			scrolls that folio's view to make it visible, switching the
			active MDI subwindow if the source element lives on a
			different folio than this reference item. No-op if the
			source element can no longer be found.
		*/
		void jumpToSource() const;

		/**
			@brief existsReferenceFor
			@param project the project to search across all its folios
			@param source_element_uuid the device to look for
			@param is_side_view which view (front/side) to check --
			a device may legitimately have one reference of each kind
			(e.g. a front-view box on one folio and a side-view box on
			another), just not two of the same kind.
			@return true if a CabinetLayoutReferenceItem for this exact
			(source_element_uuid, is_side_view) combination already
			exists somewhere in the project. Used both to reject a
			second placement attempt (@see
			DiagramEventAddCabinetLayoutReference) and, later, to mark
			an already-placed device in the source tree
			(@see CabinetLayoutSourceModel).
		*/
	static bool existsReferenceFor(QETProject *project,
									const QUuid &source_element_uuid,
									bool is_side_view);

	protected:
		void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

	private:
		void rebuildFont();

		QUuid m_source_element_uuid;
		bool m_is_side_view;

		QString m_label;
		qreal m_box_width  = 100;
		qreal m_box_height = 100;
		QFont m_font;
};

#endif // CABINETLAYOUTREFERENCEITEM_H