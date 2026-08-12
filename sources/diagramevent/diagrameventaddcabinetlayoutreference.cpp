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

#include "diagrameventaddcabinetlayoutreference.h"

#include "../diagram.h"
#include "../qetgraphicsitem/cabinetlayoutreferenceitem.h"
#include "../undocommand/addgraphicsobjectcommand.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>

/**
	@brief DiagramEventAddCabinetLayoutReference::DiagramEventAddCabinetLayoutReference
	@param diagram the diagram where this event must operate
	@param source_element_uuid the device this box represents
	@param is_side_view front (width x height) or side (depth x height) view
	@param initial_pos where the box starts, following the mouse from there
*/
DiagramEventAddCabinetLayoutReference::DiagramEventAddCabinetLayoutReference(
		Diagram *diagram,
		const QUuid &source_element_uuid,
		bool is_side_view,
		const QPointF &initial_pos) :
	DiagramEventInterface(diagram),
	m_reference_item(nullptr)
{
	auto *item = new CabinetLayoutReferenceItem(source_element_uuid, is_side_view);
	m_diagram->addItem(item);

	if (!item->refreshFromSource()) {
			//Source element not found (e.g. deleted between the drag
			//starting and the drop) -- nothing to place. Leave
			//m_running false; the caller checks isRunning() and must
			//not hand this interface to Diagram::setEventInterface()
			//(see class comment for why finish() can't be relied on
			//here to signal that back).
		m_diagram->removeItem(item);
		delete item;
		return;
	}

	item->setPos(initial_pos);
	m_reference_item = item;
	m_running = true;
}

/**
	@brief DiagramEventAddCabinetLayoutReference::~DiagramEventAddCabinetLayoutReference
*/
DiagramEventAddCabinetLayoutReference::~DiagramEventAddCabinetLayoutReference()
{
	if ((m_running || m_abort) && m_reference_item)
	{
		m_diagram->removeItem(m_reference_item);
		delete m_reference_item;
	}

	foreach (QGraphicsView *v, m_diagram->views())
		v->setContextMenuPolicy(Qt::DefaultContextMenu);
}

/**
	@brief DiagramEventAddCabinetLayoutReference::mousePressEvent
	Left click confirms placement at the current position; right
	click cancels (matching DiagramEventAddShape's convention).
	@param event
*/
void DiagramEventAddCabinetLayoutReference::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (Q_UNLIKELY(m_diagram->isReadOnly()) || !m_reference_item) {
		return;
	}

	if (event->button() == Qt::LeftButton)
	{
		QPointF pos = event->scenePos();
		if (event->modifiers() != Qt::ControlModifier) {
			pos = Diagram::snapToGrid(pos);
		}
		m_reference_item->setPos(pos);

		qWarning() << "mousePressEvent LEFT CLICK CONFIRM, pos=" << pos
				   << "m_reference_item=" << m_reference_item
				   << "scene items with our type before push:"
				   << [this]() {
				   	int n = 0;
				   	for (auto *it : m_diagram->items())
				   		if (it->type() == m_reference_item->type()) ++n;
				   	return n;
		}();

		m_diagram->undoStack().push(
			new AddGraphicsObjectCommand(m_reference_item, m_diagram, pos));

		m_reference_item = nullptr; //ownership now with the undo command
		m_running = false;
		emit finish();
		event->setAccepted(true);
		return;
	}

	if (event->button() == Qt::RightButton) {
		m_running = false;
		m_abort = true;
		emit finish();
		event->setAccepted(true);
	}
}

/**
	@brief DiagramEventAddCabinetLayoutReference::mouseMoveEvent
	@param event
*/
void DiagramEventAddCabinetLayoutReference::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_reference_item)
		return;

	QPointF pos = event->scenePos();
	if (event->modifiers() != Qt::ControlModifier) {
		pos = Diagram::snapToGrid(pos);
	}
	m_reference_item->setPos(pos);
	event->setAccepted(true);
}

void DiagramEventAddCabinetLayoutReference::init()
{
	foreach (QGraphicsView *v, m_diagram->views())
		v->setContextMenuPolicy(Qt::NoContextMenu);
}