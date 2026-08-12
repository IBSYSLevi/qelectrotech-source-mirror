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

#ifndef DIAGRAMEVENTADDCABINETLAYOUTREFERENCE_H
#define DIAGRAMEVENTADDCABINETLAYOUTREFERENCE_H

#include "diagrameventinterface.h"

#include <QUuid>

class CabinetLayoutReferenceItem;

/**
	@brief The DiagramEventAddCabinetLayoutReference class
	Interactive placement of a CabinetLayoutReferenceItem: the box
	follows the mouse (position only -- its size is already fully
	known from the source device's dimensions, unlike
	DiagramEventAddShape which draws while the user defines the
	second corner).

	Modeled on DiagramEventAddShape but simpler: there is no
	ghost-then-rebuild step like DiagramEventAddElement's
	construction/addElement() split (@see CabinetLayoutReferenceItem's
	class comment for why that split was a problem for the older,
	Element-based approach) -- the same item instance is used from
	construction all the way through to the pushed undo command.

	A left click confirms placement at the current position; Escape
	(handled by the DiagramEventInterface base class already) or a
	right click cancels.

	If the source element can no longer be found when this interface
	is constructed, isRunning() is false immediately and no item is
	ever added to the scene. Diagram::setEventInterface() only
	connects the finish() signal *after* assignment, so emitting
	finish() synchronously from inside the constructor would be lost;
	callers must therefore check isRunning() themselves right after
	construction and, if false, delete the interface directly instead
	of handing it to Diagram::setEventInterface().
*/
class DiagramEventAddCabinetLayoutReference : public DiagramEventInterface
{
	Q_OBJECT

	public:
		DiagramEventAddCabinetLayoutReference(Diagram *diagram,
											   const QUuid &source_element_uuid,
											   bool is_side_view,
											   const QPointF &initial_pos);
		~DiagramEventAddCabinetLayoutReference() override;

		void mousePressEvent   (QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent    (QGraphicsSceneMouseEvent *event) override;
		void init() override;

	private:
		CabinetLayoutReferenceItem *m_reference_item;
};

#endif // DIAGRAMEVENTADDCABINETLAYOUTREFERENCE_H