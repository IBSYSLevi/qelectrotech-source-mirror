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
#include "cabinetlayoutboxfactory.h"

#include "../../ElementsCollection/elementslocation.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <QUuid>
#include "../../qetapp.h"

CabinetLayoutBoxFactory::CabinetLayoutBoxFactory()
{
}

CabinetLayoutBoxFactory::~CabinetLayoutBoxFactory()
{
	//Nothing to clean up: generated boxes live under the custom
	//collection's "disposition" subfolder, not a temp directory --
	//see buildBoxLocation() for why they are deliberately not deleted.
}

ElementsLocation CabinetLayoutBoxFactory::buildBoxLocation(
	const QString &source_uuid,
	const QString &label,
	const QString &width_mm,
	const QString &height_mm,
	const QString &depth_mm,
	qreal scale_px_per_mm,
	bool is_side_view)
{
	bool w_ok = false, h_ok = false;
	const qreal real_width_mm  = (is_side_view ? depth_mm : width_mm).toDouble(&w_ok);
	const qreal real_height_mm = height_mm.toDouble(&h_ok);

	if (!h_ok || real_height_mm <= 0.0)
		return ElementsLocation();
	if (!w_ok || real_width_mm <= 0.0)
		return ElementsLocation();

	if (scale_px_per_mm <= 0.0)
		scale_px_per_mm = 2.0; //fall back to the library's informal default

		//<definition width/height/hotspot_*> are read back as integers
		//by Element::buildFromXml() (QET::attributeIsAnInteger), so the
		//scaled size must be rounded here.
	const int box_width  = qMax(10, qRound(real_width_mm  * scale_px_per_mm));
	const int box_height = qMax(10, qRound(real_height_mm * scale_px_per_mm));
		//Hotspot centered on the box: rotating the placed box (e.g. to
		//test a different orientation on the folio) then pivots around
		//its middle instead of a corner.
	const int hotspot_x = box_width  / 2;
	const int hotspot_y = box_height / 2;

	QDomDocument doc;
	QDomElement definition = doc.createElement(QStringLiteral("definition"));
	definition.setAttribute(QStringLiteral("type"), QStringLiteral("element"));
	definition.setAttribute(QStringLiteral("link_type"), QStringLiteral("simple"));
	definition.setAttribute(QStringLiteral("width"), box_width);
	definition.setAttribute(QStringLiteral("height"), box_height);
	definition.setAttribute(QStringLiteral("hotspot_x"), hotspot_x);
	definition.setAttribute(QStringLiteral("hotspot_y"), hotspot_y);
	doc.appendChild(definition);

	QDomElement names = doc.createElement(QStringLiteral("names"));
	QDomElement name = doc.createElement(QStringLiteral("name"));
	name.setAttribute(QStringLiteral("lang"), QStringLiteral("fr"));
	QDomText name_text = doc.createTextNode(
		label.isEmpty() ? QObject::tr("Disposition") : label);
	name.appendChild(name_text);
	names.appendChild(name);
	definition.appendChild(names);

		//"label" feeds the box's own BMK text (dynamic_text below,
		//text_from="ElementInfo"/info_name="label") -- without this,
		//that field would read an empty elementInformations() and
		//display nothing.
		//"cabinet_layout_source_uuid" is not a field the user is meant
		//to see or edit (it is intentionally not registered in
		//QETInformation::elementInfoKeys(), so it never appears in the
		//ordinary element/selection property dialogs); it exists so a
		//later "refresh cabinet layout" action can find every placed
		//box that represents a given device and regenerate it if that
		//device's width/height/depth changes. DiagramContext's XML
		//(de)serialization is schema-less -- it writes/reads whatever
		//keys are present, with no need to declare this one anywhere
		//else first.
	QDomElement infos = doc.createElement(QStringLiteral("elementInformations"));
	auto addInfo = [&doc, &infos](const QString &name, const QString &value) {
		QDomElement info = doc.createElement(QStringLiteral("elementInformation"));
		info.setAttribute(QStringLiteral("name"), name);
		info.setAttribute(QStringLiteral("show"), 1);
		QDomText text = doc.createTextNode(value);
		info.appendChild(text);
		infos.appendChild(info);
	};
	addInfo(QStringLiteral("label"), label);
	addInfo(QStringLiteral("cabinet_layout_source_uuid"), source_uuid);
	definition.appendChild(infos);

	QDomElement description = doc.createElement(QStringLiteral("description"));
	definition.appendChild(description);

		//The box outline itself: a plain rectangle spanning the full
		//size, positioned relative to the hotspot like every other
		//primitive coordinate in this format.
	QDomElement rect = doc.createElement(QStringLiteral("rect"));
	rect.setAttribute(QStringLiteral("x"), -hotspot_x);
	rect.setAttribute(QStringLiteral("y"), -hotspot_y);
	rect.setAttribute(QStringLiteral("width"), box_width);
	rect.setAttribute(QStringLiteral("height"), box_height);
	rect.setAttribute(QStringLiteral("rx"), 0);
	rect.setAttribute(QStringLiteral("ry"), 0);
	rect.setAttribute(QStringLiteral("antialias"), QStringLiteral("false"));
	rect.setAttribute(QStringLiteral("style"),
		QStringLiteral("line-style:normal;line-weight:normal;filling:none;color:black"));
	description.appendChild(rect);

		//BMK label, centered in the box. keep_visual_rotation so it
		//stays upright if the box is later rotated on the folio, and
		//rotation_point_center so that rotation pivots around this
		//text field's own center -- see rotationPointCenter/
		//keepVisualRotation elsewhere in this codebase for the same
		//mechanism.
		//
		//Important: text_width/text_height must always match the
		//box's real width_/height (not swapped for orientation),
		//because with rotation_point_center the pivot is this text
		//item's own center -- x=-hotspot_x, y=-hotspot_y combined with
		//width=box_width, height=box_height is exactly what makes that
		//center coincide with the box's true center (0,0 in
		//hotspot-relative coordinates). Swapping the axes to give a
		//narrow box's label more "flow length" breaks that alignment
		//and makes the rotated text drift off-center.
		//
		//For a narrow box (taller than wide, e.g. a slim sensor), only
		//the rotation itself is set to 270 deg so the label reads
		//bottom-to-top; the smaller font compensates for the reduced
		//space instead of trying to swap dimensions.
	const bool is_narrow = box_width < box_height;
	const int text_rotation = is_narrow ? 270 : 0;
	const int cross_axis = is_narrow ? box_width : box_height;
		//Shrink the font for a narrow box so the label has a realistic
		//chance of fitting across its short axis; clamped so it never
		//grows past the default 7pt used for a normal, wide box.
	const int font_size = is_narrow ? qBound(4, cross_axis / 8, 7) : 7;

	QDomElement text = doc.createElement(QStringLiteral("dynamic_text"));
	text.setAttribute(QStringLiteral("uuid"),
		QStringLiteral("{%1}").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
	text.setAttribute(QStringLiteral("x"), -hotspot_x);
	text.setAttribute(QStringLiteral("y"), -hotspot_y);
	text.setAttribute(QStringLiteral("text_width"), box_width);
	text.setAttribute(QStringLiteral("text_height"), box_height);
	text.setAttribute(QStringLiteral("z"), 10);
	text.setAttribute(QStringLiteral("rotation"), text_rotation);
	text.setAttribute(QStringLiteral("frame"), QStringLiteral("false"));
	text.setAttribute(QStringLiteral("keep_visual_rotation"), QStringLiteral("true"));
	text.setAttribute(QStringLiteral("rotation_point_center"), QStringLiteral("true"));
	text.setAttribute(QStringLiteral("Halignment"), QStringLiteral("AlignHCenter"));
	text.setAttribute(QStringLiteral("Valignment"), QStringLiteral("AlignVCenter"));
	text.setAttribute(QStringLiteral("font"),
		QStringLiteral("Sans Serif,%1,-1,5,0,0,0,0,0,0,normal").arg(font_size));
	text.setAttribute(QStringLiteral("text_from"), QStringLiteral("ElementInfo"));

	QDomElement text_value = doc.createElement(QStringLiteral("text"));
	text.appendChild(text_value);
	QDomElement info_name = doc.createElement(QStringLiteral("info_name"));
	QDomText info_name_text = doc.createTextNode(QStringLiteral("label"));
	info_name.appendChild(info_name_text);
	text.appendChild(info_name);

	description.appendChild(text);

		//Written into a dedicated subfolder of the *custom* (user)
		//collection, not a system temp directory: ElementsLocation's
		//path parser (setPath()) only recognises paths under one of
		//the known collection roots (common/company/macros/custom) --
		//anything else leaves m_collection_path empty, which makes
		//isElement() and exist() both report false even though the
		//file itself is perfectly valid. See the class comment for
		//why a real, recognised file is required at all.
		//
		//The file only needs to survive long enough for
		//DiagramEventAddElement to import it into the project (which
		//happens synchronously); nothing here relies on it staying
		//around afterwards, but it isn't actively deleted either, to
		//avoid ever unlinking a file while some code path might still
		//be reading it.
	if (!m_boxes_dir_ready) {
		const QString dir_path = QETApp::customElementsDirN() + "/disposition";
		QDir().mkpath(dir_path);
		m_boxes_dir = dir_path;
		m_boxes_dir_ready = true;
	}
	if (m_boxes_dir.isEmpty()) {
		qWarning() << "CabinetLayoutBoxFactory: could not prepare box directory";
		return ElementsLocation();
	}

		//A UUID rather than an incrementing counter: m_counter resets
		//to 0 every time a new DiagramView is created (e.g. reopening
		//the project), while generated files under m_boxes_dir are
		//deliberately never deleted -- a counter-based name could
		//collide with a leftover file from an earlier session, giving
		//the SAME import path a DIFFERENT element UUID and breaking
		//the project database's UNIQUE constraint on element_uuid.
		//A fresh UUID per box makes that collision structurally
		//impossible regardless of session history.
	const QString file_path = m_boxes_dir + "/cabinet_layout_box_"
		+ QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".elmt");

	QFile file(file_path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qWarning() << "CabinetLayoutBoxFactory: cannot open" << file_path
				   << file.errorString();
		return ElementsLocation();
	}

	QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	stream.setCodec("UTF-8");
#else
	stream.setEncoding(QStringConverter::Utf8);
#endif
	doc.save(stream, 4);
	file.close();

	return ElementsLocation(file_path);
}