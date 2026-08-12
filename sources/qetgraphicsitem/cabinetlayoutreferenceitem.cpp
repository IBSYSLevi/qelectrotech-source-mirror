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

#include "cabinetlayoutreferenceitem.h"

#include "../diagram.h"
#include "../qetproject.h"
#include "../dataBase/projectdatabase.h"
#include "../elementprovider.h"
#include "../qetgraphicsitem/element.h"

#include <QPainter>
#include <QSqlQuery>
#include <QSqlError>

CabinetLayoutReferenceItem::CabinetLayoutReferenceItem(
		const QUuid &source_element_uuid,
		bool is_side_view,
		QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	m_source_element_uuid(source_element_uuid),
	m_is_side_view(is_side_view)
{
	setFlags(QGraphicsItem::ItemIsMovable
			 | QGraphicsItem::ItemIsSelectable);
	setAcceptHoverEvents(true);
	setZValue(10);
	m_font = QFont(QStringLiteral("Sans Serif"), 7);
}

CabinetLayoutReferenceItem::~CabinetLayoutReferenceItem()
{
}

QRectF CabinetLayoutReferenceItem::boundingRect() const
{
	return QRectF(0, 0, m_box_width, m_box_height);
}

/**
	@brief CabinetLayoutReferenceItem::rebuildFont
	Shrinks the font for a narrow box so the label has a realistic
	chance of fitting across its short axis; mirrors the sizing rule
	previously used by CabinetLayoutBoxFactory's generated dynamic_text.
*/
void CabinetLayoutReferenceItem::rebuildFont()
{
	const bool is_narrow = m_box_width < m_box_height;
	const qreal cross_axis = is_narrow ? m_box_width : m_box_height;
	const int font_size = is_narrow
			? qBound(4, int(cross_axis / 8), 7)
			: 7;
	m_font.setPointSize(font_size);
}

void CabinetLayoutReferenceItem::paint(
		QPainter *painter,
		const QStyleOptionGraphicsItem *options,
		QWidget *)
{
	Q_UNUSED(options)

	painter->save();

	QPen pen(Qt::black);
	pen.setCosmetic(true);
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	painter->drawRect(boundingRect());

	painter->setFont(m_font);
	const bool is_narrow = m_box_width < m_box_height;
	if (is_narrow) {
		painter->save();
		painter->translate(boundingRect().center());
		painter->rotate(270);
		QRectF rotated_rect(-m_box_height / 2, -m_box_width / 2,
							 m_box_height, m_box_width);
		painter->drawText(rotated_rect, Qt::AlignCenter, m_label);
		painter->restore();
	} else {
		painter->drawText(boundingRect(), Qt::AlignCenter, m_label);
	}

	if (isSelected() || isHovered()) {
		pen.setStyle(Qt::DashLine);
		pen.setColor(Qt::blue);
		painter->setPen(pen);
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(boundingRect().adjusted(1, 1, -1, -1));
	}

	painter->restore();
}

/**
	@brief CabinetLayoutReferenceItem::refreshFromSource
	@return see header.
*/
bool CabinetLayoutReferenceItem::refreshFromSource()
{
	Diagram *dia = diagram();
	if (!dia || !dia->project() || !dia->project()->dataBase())
		return false;

	QSqlQuery query = dia->project()->dataBase()->newQuery();
	query.prepare(
		"SELECT label, width, height, depth FROM element_nomenclature_view "
		"WHERE element_uuid = :uuid"
	);
	query.bindValue(QStringLiteral(":uuid"), m_source_element_uuid.toString());
	if (!query.exec() || !query.next()) {
		qWarning() << "CabinetLayoutReferenceItem::refreshFromSource: "
					  "source element not found for uuid"
				   << m_source_element_uuid;
		return false;
	}

	prepareGeometryChange();

	m_label = query.value("label").toString();
	if (m_label.isEmpty())
		m_label = QObject::tr("(sans label)");

	bool w_ok = false, h_ok = false;
	const QString width_mm_str  = query.value("width").toString();
	const QString height_mm_str = query.value("height").toString();
	const QString depth_mm_str  = query.value("depth").toString();

	const qreal real_width_mm  = (m_is_side_view ? depth_mm_str : width_mm_str).toDouble(&w_ok);
	const qreal real_height_mm = height_mm_str.toDouble(&h_ok);

	qreal scale = dia->cabinetLayoutScale();
	if (scale <= 0.0)
		scale = 2.0; //fall back to the library's informal default

	m_box_width  = (w_ok && real_width_mm  > 0.0) ? real_width_mm  * scale : 20.0;
	m_box_height = (h_ok && real_height_mm > 0.0) ? real_height_mm * scale : 20.0;

	rebuildFont();
	update();

	return true;
}

/**
	@brief CabinetLayoutReferenceItem::jumpToSource
	See header.
*/
void CabinetLayoutReferenceItem::jumpToSource() const
{
	Diagram *dia = diagram();
	if (!dia || !dia->project())
		return;

	ElementProvider provider(dia->project());
	const QList<Element *> found = provider.fromUuids({m_source_element_uuid});
	if (found.isEmpty()) {
		qWarning() << "CabinetLayoutReferenceItem::jumpToSource: "
					  "source element not found for uuid"
				   << m_source_element_uuid;
		return;
	}

	Element *source = found.first();
	if (!source->diagram())
		return;

	source->diagram()->showMe();
	source->diagram()->clearSelection();
	source->setSelected(true);

	if (!source->diagram()->views().isEmpty())
		source->diagram()->views().first()->centerOn(source);
}

void CabinetLayoutReferenceItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event)
	jumpToSource();
}

/**
	@brief CabinetLayoutReferenceItem::existsReferenceFor
	See header.
*/
bool CabinetLayoutReferenceItem::existsReferenceFor(
		QETProject *project,
		const QUuid &source_element_uuid,
		bool is_side_view)
{
	if (!project)
		return false;

	for (Diagram *dia : project->diagrams()) {
		for (QGraphicsItem *item : dia->items()) {
			if (item->type() != CabinetLayoutReferenceItem::Type)
				continue;

			auto *reference = static_cast<CabinetLayoutReferenceItem *>(item);
			if (reference->sourceElementUuid() == source_element_uuid
				&& reference->isSideView() == is_side_view) {
				return true;
				}
		}
	}
	return false;
}

/**
	@brief CabinetLayoutReferenceItem::toXml
	Deliberately minimal compared to Element::toXml(): only what
	can't be recomputed from the project database is stored. Label
	and current width/height are looked up fresh via
	refreshFromSource() every time this item is loaded, rather than
	cached here -- that's the whole point of this item being a live
	reference rather than a snapshot.
*/
QDomElement CabinetLayoutReferenceItem::toXml(QDomDocument &document) const
{
	QDomElement element = document.createElement(QStringLiteral("cabinetLayoutReference"));
	element.setAttribute(QStringLiteral("source_uuid"), m_source_element_uuid.toString());
	element.setAttribute(QStringLiteral("side_view"),
						  m_is_side_view ? QStringLiteral("true") : QStringLiteral("false"));
	element.setAttribute(QStringLiteral("x"), QString::number(pos().x()));
	element.setAttribute(QStringLiteral("y"), QString::number(pos().y()));
	element.setAttribute(QStringLiteral("z"), QString::number(zValue()));
	element.setAttribute(QStringLiteral("rotation"), QString::number(rotation()));
	return element;
}

bool CabinetLayoutReferenceItem::fromXml(const QDomElement &dom_element)
{
	if (dom_element.tagName() != QLatin1String("cabinetLayoutReference"))
		return false;

	m_source_element_uuid = QUuid(dom_element.attribute(QStringLiteral("source_uuid")));
	m_is_side_view = dom_element.attribute(QStringLiteral("side_view")) == QLatin1String("true");

	QGraphicsObject::setPos(dom_element.attribute(QStringLiteral("x")).toDouble(),
							 dom_element.attribute(QStringLiteral("y")).toDouble());
	setZValue(dom_element.attribute(QStringLiteral("z"), QString::number(zValue())).toDouble());
	setRotation(dom_element.attribute(QStringLiteral("rotation"), QStringLiteral("0")).toDouble());

	return refreshFromSource();
}