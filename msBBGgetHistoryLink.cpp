/* To launch this program from within Mathematica use:
 *   In[1]:= link = Install["msBBGgetHistoryLink"]
 *
 * Or, launch this program from a shell and establish a
 * peer-to-peer connection.  When given the prompt Create Link:
 * type a port name. ( On Unix platforms, a port name is a
 * number less than 65536.  On Mac or Windows platforms,
 * it's an arbitrary word.)
 * Then, from within Mathematica use:
 *   In[1]:= link = Install["portname", LinkMode->Connect]
 */

#include "wstp.h"

#include <blpapi_defs.h>
#include <blpapi_event.h>
#include <blpapi_element.h>
#include <blpapi_eventdispatcher.h>
#include <blpapi_exception.h>
#include <blpapi_logging.h>
#include <blpapi_message.h>
#include <blpapi_name.h>
#include <blpapi_request.h>
#include <blpapi_session.h>
#include <blpapi_subscriptionlist.h> // correlationID should be in here

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <stdlib.h>
#include <string.h>

using namespace BloombergLP;
using namespace blpapi;
using namespace std;

namespace {
	const Name SECURITY_DATA("securityData");
	const Name SECURITY("security");
	const Name FIELD_DATA("fieldData");
	const Name SEQ_NUMBER("sequenceNumber");
	const Name RESPONSE_ERROR("responseError");
	const Name SECURITY_ERROR("securityError");
	const Name FIELD_EXCEPTIONS("fieldExceptions");
	const Name FIELD_ID("fieldId");
	const Name ERROR_INFO("errorInfo");
	const Name CATEGORY("category");
	const Name MESSAGE("message");
	const Name REASON("reason");
	const Name SESSION_TERMINATED("SessionTerminated");
	const Name SESSION_STARTUP_FAILURE("SessionStartupFailure");
};

extern "C" {
	void loggingCallback(blpapi_UInt64_t    threadId,
		int                severity,
		blpapi_Datetime_t  timestamp,
		const char        *category,
		const char        *message);
}

void loggingCallback(blpapi_UInt64_t    threadId,
	int                severity,
	blpapi_Datetime_t  timestamp,
	const char        *category,
	const char        *message)
{
	std::stringstream outstream;
	std::string severityString;
	switch (severity) {
		// The following cases will not happen if callback registered at OFF
	case blpapi_Logging_SEVERITY_FATAL:
	{
		severityString = "FATAL";
	} break;
	// The following cases will not happen if callback registered at FATAL
	case blpapi_Logging_SEVERITY_ERROR:
	{
		severityString = "ERROR";
	} break;
	// The following cases will not happen if callback registered at ERROR
	case blpapi_Logging_SEVERITY_WARN:
	{
		severityString = "WARN";
	} break;
	// The following cases will not happen if callback registered at WARN
	case blpapi_Logging_SEVERITY_INFO:
	{
		severityString = "INFO";
	} break;
	// The following cases will not happen if callback registered at INFO
	case blpapi_Logging_SEVERITY_DEBUG:
	{
		severityString = "DEBUG";
	} break;
	// The following case will not happen if callback registered at DEBUG
	case blpapi_Logging_SEVERITY_TRACE:
	{
		severityString = "TRACE";
	} break;

	};
	std::stringstream sstream;
	sstream << category << " [" << severityString << "] Thread ID = "
		<< threadId << ": " << message << std::endl;
	outstream << sstream.str() << std::endl;;
	// return outstream.str();
}

//from BBG 2016
class RefDataExample
{
	std::string              d_host;
	int                      d_port;
	std::vector<std::string> d_securities;
	std::vector<std::string> d_fields;

	bool bloombergLists(const char* ticker, const char* field)	// formerly bool parseCommandLine(int argc, char **argv) from RefDataExample.cpp
	{

		int verbosityCount = 0;

		// for the moment assumes a single ticker and single field
		d_securities.push_back(ticker);
		d_fields.push_back(field);

/*		for (int i = 1; i < argc; ++i) {
			if (!std::strcmp(argv[i], "-s") && i + 1 < argc) {
				d_securities.push_back(argv[++i]);
			}
			else if (!std::strcmp(argv[i], "-f") && i + 1 < argc) {
				d_fields.push_back(argv[++i]);
			}
			else if (!std::strcmp(argv[i], "-ip") && i + 1 < argc) {
				d_host = argv[++i];
			}
			else if (!std::strcmp(argv[i], "-p") && i + 1 < argc) {
				d_port = std::atoi(argv[++i]);
			}
			else if (!std::strcmp(argv[i], "-v")) {
				++verbosityCount;

			}
			else {
				printUsage();
				return false;
			}
		}	*/
		if (verbosityCount) {
			registerCallback(verbosityCount);
		}
		// handle default arguments
		if (d_securities.size() == 0) {
			d_securities.push_back("IBM US Equity");
		}

		if (d_fields.size() == 0) {
			d_fields.push_back("PX_LAST");
		}

		return true;
	}

	std::string printErrorInfo(const char *leadingStr, const Element &errorInfo)
	{
		std::stringstream outstream;

		outstream << leadingStr
			<< errorInfo.getElementAsString(CATEGORY)
			<< " ("
			<< errorInfo.getElementAsString(MESSAGE)
			<< ")" << std::endl;

		return outstream.str();
	}

	void registerCallback(int verbosityCount)
	{
		blpapi_Logging_Severity_t severity = blpapi_Logging_SEVERITY_OFF;
		switch (verbosityCount) {
		case 1: {
			severity = blpapi_Logging_SEVERITY_INFO;
		}break;
		case 2: {
			severity = blpapi_Logging_SEVERITY_DEBUG;
		}break;
		default: {
			severity = blpapi_Logging_SEVERITY_TRACE;
		}
		};
		blpapi_Logging_registerCallback(loggingCallback, severity);
	}


	// per the Bloomberg documentation, a messageIterator can have multiple messages, each of which is also an element and can have sub-elements. Each element can be an array.
	// securityData is typically the most interesting element, and it is always (?) an array.
	std::string sendRefDataRequest(Session &session, const char* startDate, const char* endDate, const char* periodicitySelection, const char* periodicityAdjustment, int useDPDF, int debug)
	{
		std::stringstream outstream;
		Service refDataService = session.getService("//blp/refdata");
		Request request = refDataService.createRequest("HistoricalDataRequest");

		// Add securities to request
		Element securities = request.getElement("securities");
		for (size_t i = 0; i < d_securities.size(); ++i) {
			securities.appendValue(d_securities[i].c_str());
		}

		// Add fields to request
		Element fields = request.getElement("fields");
		for (size_t i = 0; i < d_fields.size(); ++i) {
			fields.appendValue(d_fields[i].c_str());
		}
		request.set("periodicityAdjustment", periodicityAdjustment);
		request.set("periodicitySelection", periodicitySelection);
		request.set("startDate", startDate);
		request.set("endDate", endDate);
//		request.set("returnNullValue", "True");	// this is useful for handling aberrant cases in msGetBBG(), not sure if it's needed here.
		 if (useDPDF == 0)
		 {
			request.set("adjustmentFollowDPDF", false); }
		else
		{
			request.set("adjustmentFollowDPDF", true); }

		outstream << "Sending Request: " << request << std::endl;
		session.sendRequest(request);
		return outstream.str();
	}

	// return true if processing is completed, false otherwise
	std::string processResponseEvent(Event event, int debug)
	{
		std::stringstream outstream;
		MessageIterator msgIter(event);

		while (msgIter.next()) {
			Message msg = msgIter.message();
			Element securities = msg.asElement();
			if (securities.numElements()>0) {
				Element securityData = msg.getElement(SECURITY_DATA); //get security
				Element fieldDataArray = securityData.getElement(FIELD_DATA);
				outstream << "{";
				for (size_t j = 0; j <fieldDataArray.numValues(); ++j) {
					Element fieldData = fieldDataArray.getValueAsElement(j); //get each element in the security, every single data point (date, data) is an element
					for (size_t k = 0; k < fieldData.numElements(); k = k + 2) {
						outstream << "{\"";
						Element field1 = fieldData.getElement(k); //date is an element
						Element field2 = fieldData.getElement(k + 1); //data is an element
						outstream << field1.getValueAsString();
						outstream << "\", ";
						outstream << field2.getValueAsString();
						outstream << "}";
					}
					if (j < fieldDataArray.numValues() - 1)
						outstream << ",";
				}
				outstream << "}";
			}
		}
		return outstream.str();
	}

	std::string eventLoop(Session &session, int debug)
	{
		std::stringstream outstream;
		bool done = false;
		while (!done) {
			Event event = session.nextEvent();
			if (event.eventType() == Event::PARTIAL_RESPONSE) {
				if (debug > 0) {
					outstream << "Processing Partial Response" << std::endl;
				}
				outstream << processResponseEvent(event, debug);
			}
			else if (event.eventType() == Event::RESPONSE) {
				if (debug > 0) {
					outstream << "Processing Response" << std::endl;
				}
				outstream << processResponseEvent(event, debug);
				done = true;
			}
			else {
				MessageIterator msgIter(event);
				while (msgIter.next()) {
					Message msg = msgIter.message();
					if (event.eventType() == Event::REQUEST_STATUS) {
						outstream << "REQUEST FAILED: " << msg.getElement(REASON) << std::endl;
						done = true;
					}
					else if (event.eventType() == Event::SESSION_STATUS) {
						if (msg.messageType() == SESSION_TERMINATED ||
							msg.messageType() == SESSION_STARTUP_FAILURE) {
							outstream << "session terminated";
							done = true;
						}
					}
				}
			}
		}
		return outstream.str();
	}


public:
	
	RefDataExample()
	{
		d_host = "localhost";
		d_port = 8194;
	}

	~RefDataExample()
	{
	}

	// makes one or more Bloomberg calls, gets the Event response and associated MessageIterator, then calls ProcessMessage() to deal with what it's gotten. Returns the result to msGetBBG()
	// adapted from BBG 2016, closely related to the "Retrieve" function in the 2008/2009 code
	std::string run(const char* ticker, const char* field, const char* startDate, const char* endDate, const char* periodicitySelection, const char* periodicityAdjustment, int useDPDF, int debug)
	{
		std::stringstream outstream;
		bloombergLists(ticker, field); /* sets up tickers and fields */

		SessionOptions sessionOptions;
		sessionOptions.setServerHost(d_host.c_str());
		sessionOptions.setServerPort(d_port);

		if (debug > 0) { outstream << "Connecting to " + d_host + ":" << d_port << std::endl; }
		Session session(sessionOptions);
		if (!session.start()) {
			outstream << "Failed to start session." << std::endl;
			return outstream.str();
		}
		if (!session.openService("//blp/refdata")) {
			outstream << "Failed to open //blp/refdata" << std::endl;
			return outstream.str();
		}
		sendRefDataRequest(session, startDate, endDate, periodicitySelection, periodicityAdjustment, useDPDF, debug);


		// wait for events from session.
		try {
			outstream << eventLoop(session, debug);
		}
		catch (Exception &e) {
			outstream << "Library Exception !!!"
				<< e.description()
				<< std::endl;
		}
		catch (...) {
			outstream << "Unknown Exception !!!" << std::endl;
		}

		session.stop();
		return outstream.str();
	}
};

// This is effectively our entry point. It's called by WSTP template code.
// Per the template file this has a returntype of Manual, which means I need to use a function like WSPutInteger32() to return an integer or WSPutString() to return a string.
void msBBGgetHistoryLink(const char* ticker, const char* field, const char* startDate, const char* endDate, const char* periodicitySelection = "DAILY", const char* periodicityAdjustment = "Actual", int useDPDF=0, int debug=0)
{
	RefDataExample bbgCallResponse;
	std::string bbgOutput;
	bbgOutput = bbgCallResponse.run(ticker, field, startDate, endDate, periodicitySelection, periodicityAdjustment, useDPDF, debug);
	WSPutString(stdlink, bbgOutput.c_str()/* std::string converted to const char */);	// https://reference.wolfram.com/language/ref/c/WSPutString.html

	return;
}


// the following is from addTwo
#if WINDOWS_WSTP

#if __BORLANDC__
#pragma argsused
#endif

int PASCAL WinMain( HINSTANCE hinstCurrent, HINSTANCE hinstPrevious, LPSTR lpszCmdLine, int nCmdShow)
{
	char  buff[512];
	char FAR * buff_start = buff;
	char FAR * argv[32];
	char FAR * FAR * argv_end = argv + 32;

	hinstPrevious = hinstPrevious; /* suppress warning */

	if( !WSInitializeIcon( hinstCurrent, nCmdShow)) return 1;
	WSScanString( argv, &argv_end, &lpszCmdLine, &buff_start);
	return WSMain( (int)(argv_end - argv), argv);
}

#else

int main(int argc, char* argv[])
{
	return WSMain(argc, argv);
}

#endif
