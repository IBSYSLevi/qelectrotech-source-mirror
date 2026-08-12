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

#ifndef CABINETLAYOUTBOXFACTORY_H
#define CABINETLAYOUTBOXFACTORY_H

#include <QString>

class ElementsLocation;

/**
	@brief The CabinetLayoutBoxFactory class
	Synthesizes a .elmt symbol representing the real-world footprint of
	a device already placed in the project, for use as a scale-accurate
	box in a cabinet layout ("disposition des armoires") folio.

	Element::Element() unconditionally requires an ElementsLocation
	whose exist() is true (it checks this before ever looking at the
	XML), so there is no way to hand it an in-memory-only definition --
	a real file backing the location is required by the current
	architecture. Beyond that, ElementsLocation::setPath() only
	recognises paths under one of the known collection roots (common,
	company, macros, custom); anything else (e.g. a plain system temp
	directory) leaves its internal collection path empty, silently
	making isElement()/exist() both report false. This class therefore
	writes generated boxes into a dedicated subfolder of the *custom*
	(user) collection rather than a system temp directory.
*/
class CabinetLayoutBoxFactory
{
	public:
		CabinetLayoutBoxFactory();
		~CabinetLayoutBoxFactory();

		/**
			@brief buildBoxLocation
			@param source_uuid the UUID of the device this box represents,
			stored on the generated box (as elementInformation
			cabinet_layout_source_uuid) so a later "refresh" action can
			find and regenerate it if the source's dimensions change.
			@param label the BMK/label to display centered in the box
			@param width_mm the device's real-world width (front view) or unused (side view)
			@param height_mm the device's real-world height
			@param depth_mm the device's real-world depth (side view) or unused (front view)
			@param scale_px_per_mm QET drawing units per real-world millimeter
			@param is_side_view true for depth x height, false for width x height
			@return a valid ElementsLocation pointing at a freshly written
			.elmt file, or an invalid (default-constructed) ElementsLocation
			if neither of the two relevant dimensions is a usable
			positive number.
		*/
		ElementsLocation buildBoxLocation(
			const QString &source_uuid,
			const QString &label,
			const QString &width_mm,
			const QString &height_mm,
			const QString &depth_mm,
			qreal scale_px_per_mm,
			bool is_side_view);

	private:
		QString m_boxes_dir;
		bool m_boxes_dir_ready = false;
};

#endif // CABINETLAYOUTBOXFACTORY_H