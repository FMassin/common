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


#define SEISCOMP_COMPONENT ArclinkConnection


#include <string>
#include <set>
#include <utility>
#include <limits>
#include <cerrno>
#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

#include <seiscomp/logging/log.h>
#include <seiscomp/core/strings.h>
#include <seiscomp/io/records/mseedrecord.h>
#include "arclink.h"
// HACK to retrieve the record length
#include <libmseed.h>

#ifdef WIN32
#undef min
#undef max
#define posix_read _read
typedef int ssize_t;
#else
#define posix_read read
#endif


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
namespace Seiscomp {
namespace RecordStream {
namespace Arclink {


using namespace std;
using namespace Seiscomp;
using namespace Seiscomp::Core;
using namespace Seiscomp::IO;


const string DefaultHost = "localhost";
const string DefaultPort = "18001";


namespace {


class SC_SYSTEM_CORE_API ArclinkException: public Seiscomp::IO::RecordStreamException {
	public:
		ArclinkException(): RecordStreamException("ArcLink exception") {}
		ArclinkException(const std::string& what): RecordStreamException(what) {}
};

class SC_SYSTEM_CORE_API ArclinkCommandException: public ArclinkException {
	public:
		ArclinkCommandException(): ArclinkException("command not accepted") {}
		ArclinkCommandException(const std::string& what): ArclinkException(what) {}
};


}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
IMPLEMENT_SC_CLASS_DERIVED(ArclinkConnection,
			   Seiscomp::IO::RecordStream,
			   "ArclinkConnection");

REGISTER_RECORDSTREAM(ArclinkConnection, "arclink");
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ArclinkConnection::ArclinkConnection()
: RecordStream()
, _user("guest@anywhere")
, _readingData(false)
, _chunkMode(false)
, _remainingBytes(0) {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ArclinkConnection::ArclinkConnection(std::string source)
: RecordStream()
, _user("guest@anywhere")
, _readingData(false)
, _chunkMode(false)
, _remainingBytes(0) {
	setSource(source);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ArclinkConnection::~ArclinkConnection() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool ArclinkConnection::setSource(const string &serverloc) {
	size_t pos = serverloc.find('?');
	if ( pos != std::string::npos ) {
		_serverloc = serverloc.substr(0, pos);
		std::string params = serverloc.substr(pos+1);
		std::vector<std::string> toks;
		split(toks, params.c_str(), "&");
		if ( !toks.empty() ) {
			for ( std::vector<std::string>::iterator it = toks.begin();
			      it != toks.end(); ++it ) {
				std::string name, value;

				pos = it->find('=');
				if ( pos != std::string::npos ) {
					name = it->substr(0, pos);
					value = it->substr(pos+1);
				}
				else {
					name = *it;
					value = "";
				}

				if ( name == "user" )
					_user = value;
				else if ( name == "pwd" )
					_passwd = value;

				if ( name == "dump" )
					_dump.open(value.c_str());
			}
		}
	}
	else
		_serverloc = serverloc;

	// set address defaults if necessary
	if ( _serverloc.empty() || _serverloc == ":" )
		_serverloc = DefaultHost + ":" + DefaultPort;
	else {
		pos = _serverloc.find(':');
		if ( pos == string::npos )
			_serverloc += ":" + DefaultPort;
		else if ( pos == _serverloc.length()-1 )
			_serverloc += DefaultPort;
		else if ( pos == 0 )
			_serverloc.insert(0, DefaultHost);
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool ArclinkConnection::setRecordType(const char* type) {
	return !strcmp(type, "mseed");
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool ArclinkConnection::setUser(std::string name, std::string password) {
	_user = name;
	_passwd = password;
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool ArclinkConnection::addStream(const std::string &net, const std::string &sta,
                                  const std::string &loc, const std::string &cha) {
	pair<set<StreamIdx>::iterator, bool> result;
	result = _streams.insert(StreamIdx(net, sta, loc, cha));
	if ( result.second ) _ordered.push_back(*result.first);
	return result.second;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool ArclinkConnection::addStream(const std::string &net, const std::string &sta,
                                  const std::string &loc, const std::string &cha,
                                  const OPT(Seiscomp::Core::Time) &stime,
                                  const OPT(Seiscomp::Core::Time) &etime) {
	pair<set<StreamIdx>::iterator, bool> result;
	result = _streams.insert(StreamIdx(net, sta, loc, cha, stime, etime));
	if ( result.second ) _ordered.push_back(*result.first);
	return result.second;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool ArclinkConnection::setStartTime(const OPT(Seiscomp::Core::Time) &stime) {
	_stime = stime;
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool ArclinkConnection::setEndTime(const OPT(Seiscomp::Core::Time) &etime) {
	_etime = etime;
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool ArclinkConnection::setTimeout(int seconds) {
	_sock.setTimeout(seconds);
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool ArclinkConnection::clear() {
	this->~ArclinkConnection();
	new(this) ArclinkConnection(_serverloc);
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Hopefully safe to be called from another thread
void ArclinkConnection::close() {
	_sock.interrupt();

	_dump.close();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool ArclinkConnection::reconnect() {
	if ( _sock.isOpen() )
		_sock.close();

	_readingData = false;
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void ArclinkConnection::handshake() {
	_sock.sendRequest("HELLO", false);
	string r = _sock.readline();
	if ( r == "ERROR" ) {
		SEISCOMP_ERROR("Remote server did not accept HELLO");
		throw ArclinkCommandException("HELLO");
	}

	string _software = r;
	string _organization = _sock.readline();

	SEISCOMP_DEBUG("%s running at %s", _software.c_str(),
	               _organization.c_str());

	if ( _passwd.length() ) {
		_sock.sendRequest("USER " + _user + " " + _passwd, true);
	}
	else {
		_sock.sendRequest("USER " + _user, true);
	}

	_sock.sendRequest("REQUEST WAVEFORM format=MSEED", true);

	Core::Time endTime = _etime ? *_etime : Core::Time::UTC();

	for ( list<StreamIdx>::iterator it = _ordered.begin(); it != _ordered.end(); ++it ) {
		SEISCOMP_DEBUG("Arclink request: %s", it->str(_stime, endTime).c_str());
		if ((!it->startTime() && !_stime) ||
			(!it->endTime() && !endTime)) {
			/* invalid time window ignore stream */
			SEISCOMP_WARNING("... has invalid time window -> ignore this request above");
		}
		else
			_sock.sendRequest(it->str(_stime, endTime), false);
	}

	_reqID = _sock.sendRequest("END", true);
	_sock.sendRequest("BCDOWNLOAD " + _reqID, false);
	r = _sock.readline();

	if ( r == "ERROR" ) {
		_sock.sendRequest("BDOWNLOAD " + _reqID, false);
		r = _sock.readline();
	}

	if ( r == "ERROR" || r == "END" ) {
		_remainingBytes = 0;
	}
	else if ( r.compare(0, 6, "CHUNK ") == 0 ) {
		char *tail;
		_chunkMode = true;
		_remainingBytes = strtoul(r.c_str() + 6, &tail, 10);
		if ( *tail ) {
			SEISCOMP_ERROR("Invalid ArcLink response: %s", r.c_str());
			throw ArclinkException("invalid response");
		}
		SEISCOMP_DEBUG("Chunk mode detected, first chunk with %d bytes", _remainingBytes);
	}
	else {
		char *tail;
		_chunkMode = false;
		_remainingBytes = strtoul(r.c_str(), &tail, 10);
		if ( *tail ) {
			SEISCOMP_ERROR("Invalid ArcLink response: %s", r.c_str());
			throw ArclinkException("invalid response");
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void ArclinkConnection::cleanup() {
	_sock.sendRequest("PURGE " + _reqID, true);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Record *ArclinkConnection::next() {
	if ( _readingData && !_sock.isOpen() ) {
		return nullptr;
	}

	_sock.startTimer();

	if ( ! _readingData ) {
		try {
			_sock.open(_serverloc);
			handshake();
		}
		catch ( GeneralException & ) {
			_sock.close();
			return nullptr;
		}

		_readingData = true;
		if ( _remainingBytes <= 0 ) {
			_sock.close();
			return nullptr;
		}
	}

	try {
		while ( _sock.isOpen() ) {
			std::istringstream stream(std::istringstream::in|std::istringstream::binary);
			// HACK to retrieve the record length
			string data = _sock.read(RECSIZE);
			int reclen = ms_detect(data.c_str(), RECSIZE);
			if (reclen > RECSIZE)
				data += _sock.read(reclen - RECSIZE);
			else {
				if (reclen <= 0) SEISCOMP_ERROR("Retrieving the record length failed (try 512 Byte)!");
				reclen = RECSIZE;
			}

			stream.str(data);

			IO::MSeedRecord *rec = new IO::MSeedRecord();
			setupRecord(rec);
			try {
				rec->read(stream);
			}
			catch ( ... ) {
				delete rec;
				rec = nullptr;
			}

			if ( _dump ) _dump << data;

			/////////////////////////////////////
			_remainingBytes -= reclen;

			// Read next chunk size
			if ( _chunkMode && _remainingBytes <= 0 ) {
				string r = _sock.readline();
				if ( r.compare(0, 6, "CHUNK ") == 0 ) {
					char *tail;
					_remainingBytes = strtoul(r.c_str() + 6, &tail, 10);
					if ( *tail ) {
						SEISCOMP_ERROR("Invalid ArcLink response: %s", r.c_str());
						_sock.close();
					}
				}
				else
					SEISCOMP_DEBUG("Received status: %s", r.c_str());
			}

			if ( _remainingBytes <= 0 ) {
				cleanup();
				_sock.close();
			}

			if ( rec != nullptr )
				return rec;
		}
	}
	catch ( GeneralException & ) {
		_sock.close();
	}

	return nullptr;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
} // namespace Arclink
} // namespace RecordStream
} // namespace Seiscomp
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
