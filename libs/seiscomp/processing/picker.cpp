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


#define SEISCOMP_COMPONENT Picker

#include <seiscomp/processing/picker.h>
#include <seiscomp/math/mean.h>
#include <seiscomp/core/interfacefactory.ipp>
#include <seiscomp/logging/log.h>

#include <fstream>


IMPLEMENT_INTERFACE_FACTORY(Seiscomp::Processing::Picker, SC_SYSTEM_CLIENT_API);


namespace Seiscomp {
namespace Processing {

IMPLEMENT_SC_ABSTRACT_CLASS_DERIVED(Picker, TimeWindowProcessor, "Picker");
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Picker::Picker() {
	init();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Picker::Picker(const Core::Time& trigger)
 : _trigger(trigger) {
	init();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::init() {
	setMargin(Core::TimeSpan(0.0));
	_config.noiseBegin  = 0;
	_config.signalBegin = -30;
	_config.signalEnd   =  10;
	_config.snrMin      =   3;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Picker::~Picker() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::computeTimeWindow() {
	if ( !bool(_trigger) ) {
		setTimeWindow(Core::TimeWindow());
		return;
	}

	Core::Time startTime = _trigger + Core::TimeSpan(std::min(_config.noiseBegin, _config.signalBegin));
	Core::Time   endTime = _trigger + Core::TimeSpan(_config.signalEnd);

	setTimeWindow(Core::TimeWindow(startTime, endTime));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Picker::handleGap(Filter *filter, const Core::TimeSpan& span,
                       double lastSample, double nextSample,
                       size_t missingSamples) {
	setStatus(QCError, -1);
	terminate();

	//TODO: Handle gaps
	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::reset() {
	TimeWindowProcessor::reset();
	_trigger = Core::Time();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::setTrigger(const Core::Time& trigger) {
	if ( _trigger )
		throw Core::ValueException("The trigger has been set already");

	_trigger = trigger;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::finalizePick(DataModel::Pick *pick) const {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::setPublishFunction(const PublishFunc& func) {
	_func = func;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::emitPick(const Result &result) {
	if ( isEnabled() && _func )
		_func(this, result);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::process(const Record *record, const DoubleArray &) {
	// Sampling frequency has not been set yet
	if ( _stream.fsamp == 0.0 ) {
		terminate();
		return;
	}

	// Error occured while feeding a record or already finished?
	if ( isFinished() ) return;

	// Process unless the available time window does not include the
	// requested time window
	if ( !dataTimeWindow().contains(timeWindow())) return;

	// data window relative to continuous()->startTime()
	double relTriggerTime = (_trigger - dataTimeWindow().startTime()).length();

	double dt1 = relTriggerTime + _config.signalBegin;
	double dt2 = relTriggerTime + _config.signalEnd;
	double snr = -1;

	// Calculate data array indicies of requested time window
	int i1 = int(dt1*_stream.fsamp);
	int i2 = int(dt2*_stream.fsamp);

	// Calculate the initial trigger time
	int triggerIdx = int(relTriggerTime * _stream.fsamp);
	int lowerUncertainty = -1;
	int upperUncertainty = -1;
	OPT(Polarity) polarity;

	if ( !calculatePick(continuousData().size(), continuousData().typedData(),
	                    i1, i2, triggerIdx, lowerUncertainty, upperUncertainty,
	                    snr, polarity) ) {
		setStatus(Error, 0.0);
		return;
	}

	Core::Time pickTime = dataTimeWindow().startTime() + Core::TimeSpan(triggerIdx/_stream.fsamp);

	// Debug: print the time difference between the pick and the initial trigger
	SEISCOMP_DEBUG("Picker::process repick result: dt=%.3f  snr=%.2f",
	               double(pickTime - _trigger), snr);

	if ( snr >= _config.snrMin ) {
		setStatus(Finished, 100.);
		Result res;
		res.record = record;
		res.snr = snr;
		res.time = pickTime;
		res.timeLowerUncertainty = double(lowerUncertainty) / _stream.fsamp;
		res.timeUpperUncertainty = double(upperUncertainty) / _stream.fsamp;
		res.timeWindowBegin = double(timeWindow().startTime() - pickTime);
		res.timeWindowEnd = double(timeWindow().endTime() - pickTime);
		res.polarity = polarity;
		emitPick(res);
	}
	else
		setStatus(LowSNR, snr);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Picker::writeData() {
	if ( !_stream.lastRecord ) return;

	std::ofstream of((_stream.lastRecord->streamID() + "-postpick.data").c_str());

	of << "#sampleRate: " << _stream.lastRecord->samplingFrequency() << std::endl;

	for ( int i = 0; i < continuousData().size(); ++i )
		of << i << "\t" << continuousData()[i] << std::endl;
	of.close();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}

}
