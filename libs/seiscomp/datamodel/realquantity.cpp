/***************************************************************************
 * Copyright (C) gempa GmbH                                                *
 * All rights reserved.                                                    *
 * Contact: gempa GmbH (seiscomp-dev@gempa.de)                             *
 *                                                                         *
 * GNU Affero General Public License Usage                                 *
 * This file may be used under the terms of the GNU Affero                 *
 * Public License version 3.0 as published by the Free Software Foundation *
 * and appearing in the file LICENSE included in the packaging of this     *
 * file. Please review the following information to ensure the GNU Affero  *
 * Public License version 3.0 requirements will be met:                    *
 * https://www.gnu.org/licenses/agpl-3.0.html.                             *
 *                                                                         *
 * Other Usage                                                             *
 * Alternatively, this file may be used in accordance with the terms and   *
 * conditions contained in a signed written agreement between you and      *
 * gempa GmbH.                                                             *
 ***************************************************************************/


#define SEISCOMP_COMPONENT DataModel
#include <seiscomp/datamodel/realquantity.h>
#include <seiscomp/datamodel/version.h>
#include <seiscomp/datamodel/metadata.h>
#include <seiscomp/logging/log.h>


namespace Seiscomp {
namespace DataModel {


IMPLEMENT_SC_CLASS(RealQuantity, "RealQuantity");


RealQuantity::MetaObject::MetaObject(const Core::RTTI *rtti) : Seiscomp::Core::MetaObject(rtti) {
	addProperty(Core::simpleProperty("value", "float", false, false, false, false, false, false, nullptr, &RealQuantity::setValue, &RealQuantity::value));
	addProperty(Core::simpleProperty("uncertainty", "float", false, false, false, false, true, false, nullptr, &RealQuantity::setUncertainty, &RealQuantity::uncertainty));
	addProperty(Core::simpleProperty("lowerUncertainty", "float", false, false, false, false, true, false, nullptr, &RealQuantity::setLowerUncertainty, &RealQuantity::lowerUncertainty));
	addProperty(Core::simpleProperty("upperUncertainty", "float", false, false, false, false, true, false, nullptr, &RealQuantity::setUpperUncertainty, &RealQuantity::upperUncertainty));
	addProperty(Core::simpleProperty("confidenceLevel", "float", false, false, false, false, true, false, nullptr, &RealQuantity::setConfidenceLevel, &RealQuantity::confidenceLevel));
	addProperty(objectProperty<RealPDF1D>("pdf", "RealPDF1D", false, false, true, &RealQuantity::setPdf, &RealQuantity::pdf));
}


IMPLEMENT_METAOBJECT(RealQuantity)


RealQuantity::RealQuantity() {
	_value = 0;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RealQuantity::RealQuantity(const RealQuantity &other)
: Core::BaseObject() {
	*this = other;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RealQuantity::RealQuantity(double value,
                           const OPT(double)& uncertainty,
                           const OPT(double)& lowerUncertainty,
                           const OPT(double)& upperUncertainty,
                           const OPT(double)& confidenceLevel,
                           const OPT(RealPDF1D)& pdf)
: _value(value)
, _uncertainty(uncertainty)
, _lowerUncertainty(lowerUncertainty)
, _upperUncertainty(upperUncertainty)
, _confidenceLevel(confidenceLevel)
, _pdf(pdf)
{}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RealQuantity::~RealQuantity() {
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RealQuantity::operator double&() {
	return _value;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RealQuantity::operator double() const {
	return _value;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool RealQuantity::operator==(const RealQuantity &rhs) const {
	if ( !(_value == rhs._value) )
		return false;
	if ( !(_uncertainty == rhs._uncertainty) )
		return false;
	if ( !(_lowerUncertainty == rhs._lowerUncertainty) )
		return false;
	if ( !(_upperUncertainty == rhs._upperUncertainty) )
		return false;
	if ( !(_confidenceLevel == rhs._confidenceLevel) )
		return false;
	if ( !(_pdf == rhs._pdf) )
		return false;
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool RealQuantity::operator!=(const RealQuantity &rhs) const {
	return !operator==(rhs);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool RealQuantity::equal(const RealQuantity &other) const {
	return *this == other;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void RealQuantity::setValue(double value) {
	_value = value;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double RealQuantity::value() const {
	return _value;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void RealQuantity::setUncertainty(const OPT(double)& uncertainty) {
	_uncertainty = uncertainty;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double RealQuantity::uncertainty() const {
	if ( _uncertainty )
		return *_uncertainty;
	throw Seiscomp::Core::ValueException("RealQuantity.uncertainty is not set");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void RealQuantity::setLowerUncertainty(const OPT(double)& lowerUncertainty) {
	_lowerUncertainty = lowerUncertainty;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double RealQuantity::lowerUncertainty() const {
	if ( _lowerUncertainty )
		return *_lowerUncertainty;
	throw Seiscomp::Core::ValueException("RealQuantity.lowerUncertainty is not set");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void RealQuantity::setUpperUncertainty(const OPT(double)& upperUncertainty) {
	_upperUncertainty = upperUncertainty;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double RealQuantity::upperUncertainty() const {
	if ( _upperUncertainty )
		return *_upperUncertainty;
	throw Seiscomp::Core::ValueException("RealQuantity.upperUncertainty is not set");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void RealQuantity::setConfidenceLevel(const OPT(double)& confidenceLevel) {
	_confidenceLevel = confidenceLevel;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double RealQuantity::confidenceLevel() const {
	if ( _confidenceLevel )
		return *_confidenceLevel;
	throw Seiscomp::Core::ValueException("RealQuantity.confidenceLevel is not set");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void RealQuantity::setPdf(const OPT(RealPDF1D)& pdf) {
	_pdf = pdf;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RealPDF1D& RealQuantity::pdf() {
	if ( _pdf )
		return *_pdf;
	throw Seiscomp::Core::ValueException("RealQuantity.pdf is not set");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const RealPDF1D& RealQuantity::pdf() const {
	if ( _pdf )
		return *_pdf;
	throw Seiscomp::Core::ValueException("RealQuantity.pdf is not set");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
RealQuantity &RealQuantity::operator=(const RealQuantity &other) {
	_value = other._value;
	_uncertainty = other._uncertainty;
	_lowerUncertainty = other._lowerUncertainty;
	_upperUncertainty = other._upperUncertainty;
	_confidenceLevel = other._confidenceLevel;
	_pdf = other._pdf;
	return *this;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void RealQuantity::serialize(Archive &ar) {
	// Do not read/write if the archive's version is higher than
	// currently supported
	if ( ar.isHigherVersion<Version::Major,Version::Minor>() ) {
		SEISCOMP_ERROR("Archive version %d.%d too high: RealQuantity skipped",
		               ar.versionMajor(), ar.versionMinor());
		ar.setValidity(false);
		return;
	}

	ar & NAMED_OBJECT_HINT("value", _value, Archive::XML_ELEMENT | Archive::XML_MANDATORY);
	ar & NAMED_OBJECT_HINT("uncertainty", _uncertainty, Archive::XML_ELEMENT);
	ar & NAMED_OBJECT_HINT("lowerUncertainty", _lowerUncertainty, Archive::XML_ELEMENT);
	ar & NAMED_OBJECT_HINT("upperUncertainty", _upperUncertainty, Archive::XML_ELEMENT);
	ar & NAMED_OBJECT_HINT("confidenceLevel", _confidenceLevel, Archive::XML_ELEMENT);
	if ( ar.supportsVersion<0,10>() ) {
		ar & NAMED_OBJECT_HINT("pdf", _pdf, Archive::STATIC_TYPE | Archive::XML_ELEMENT);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
