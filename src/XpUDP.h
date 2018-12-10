//==================================================================================================
//  Franks Flightsim Intruments project
//  by Frank van Ierland
//
// This code is in the public domain.
//
//==================================================================================================

//==================================================================================================
// XPdefs.h
//
// author: Frank van Ierland
// version: 0.1
//
// various definitions and procedures to use xplane data thru UDP link
//
//------------------------------------------------------------------------------------
/// \author  Frank van Ierland (frank@van-ierland.com) DO NOT CONTACT THE AUTHOR DIRECTLY: USE THE LISTS
// Copyright (C) 2018 Frank van Ierland

// TODO: Code to send data back to XPlane

#ifndef XpUDP_h_
#define XpUDP_h_

#include <stdlib.h>
#include <WiFi.h>
#include <QList.h>
#include "XpUDPdataRef.h"

#include "GenericDefs.h"

#if defined(ARDUINO) && ARDUINO >= 100
#	include "Arduino.h"
#else
#	include "WProgram.h"
#endif

#include <esp_task_wdt.h>

#include <limits.h>
#include "XPUtils.h"
#include "XpPlaneInfo.h"

constexpr auto XP_BEACONTIMEOUT_MS = 10 * 1000;

#define XP_Multicast_Port 49707
#define XP_MulticastAdress_0 239
#define XP_MulticastAdress_1 255
#define XP_MulticastAdress_2 1
#define XP_MulticastAdress_3 1

// defines for msg headers used in XP UDP interface.
#define XPMSG_XXXX	"XXXX" // novalid command
#define XPMSG_BECN	"BECN"	//get beacon and inintate connection
#define XPMSG_CMND	"CMND"	// run a command
#define XPMSG_RREF	"RREF"	//SEND ME ALL THE DATAREFS I WANT: RREF
#define XPMSG_DREF	"DREF"	//SET A DATAREF TO A VALUE: DREF
#define XPMSG_DATA	"DATA"	//SET A DATA OUTPUT TO A VALUE: DATA
#define XPMSG_ALRT	"ALRT"	//MAKE AN ALERT MESSAGE IN X-PLANE: ALRT
#define XPMSG_FAIL 	"FAIL"	//FAIL A SYSTEM: FAIL
#define XPMSG_RECO 	"RECO"

constexpr auto _Xp_dref_in_msg_size = 413;
constexpr auto _Xp_rref_size = 400;
constexpr auto _Xp_dref_size = 509;

constexpr long _Xp_beacon_timeoutMs = 10 * 1000;
constexpr long _Xp_beacon_restart_timeoutMs = 10 * 1000;
constexpr long _Xp_initial_clean_timeoutMs = 10 * 1000;

typedef enum _XpCommand {
	XP_CMD_ALRT,
	XP_CMD_BECN,
	XP_CMD_CMND,
	XP_CMD_DATA,
	XP_CMD_DREF,
	XP_CMD_FAIL,
	XP_CMD_RECO,
	XP_CMD_RREF,
	XP_CMD_XXXX
};

typedef enum _XpRunStates {
	XP_RUNSTATE_STOP,
	XP_RUNSTATE_WIFI_UP,
	XP_RUNSTATE_UDP_UP,
	XP_RUNSTATE_GOT_BEACON,
	XP_RUNSTATE_IS_CLEANING,
	XP_RUNSTATE_DONE_CLEANING,
	XP_RUNSTATE_GOT_PLANE,
	XP_RUNSTATE_RUNNIG
};

typedef struct _XpCommands {
	char*		text;
	_XpCommand	code;
};

// make sure list is sorted
static const _XpCommands _myListCommands[] = {
	{"ALRT",XP_CMD_ALRT},
	{"BECN",XP_CMD_BECN},
	{"CMND",XP_CMD_CMND},
	{"DATA",XP_CMD_DATA},
	{"DREF",XP_CMD_DREF},
	{"FAIL",XP_CMD_FAIL},
	{"RECO",XP_CMD_RECO},
	{"RREF",XP_CMD_RREF},
	{"XXXX",XP_CMD_XXXX}
};

//==================================================================================================
//
//==================================================================================================
class XpUDP {
public:

	typedef enum {
		XP_ERR_SUCCESS = 0,
		XP_ERR_BEACON_NOT_FOUND = 1,
		XP_ERR_BEACON_WRONG_VERSION = 2,
		XP_ERR_WRONG_HEADER = 3,
		XP_ERR_BUFFER_OVERFLOW = 4,
		XP_ERR_NO_DATA = 5,
		XP_ERR_NO_PLANE_SET_YET = 6,
		XP_ERR_REFERENCE_NOT_FOUND = 7,
		XP_ERR_FAILED = 8,
		XP_ERR_NOT_FOUND
	} XpErrorCode;

	//constructor
	XpUDP(void(*callbackFunc)(uint16_t canId, float par), void(*setStateFunc)(bool state));
	~XpUDP() {};
	// start connection
	int start();   // do we still need this

	int dataReader();				// poll for new data need to be in loop()
	static short getState();

	int registerDataRef(uint16_t canID, bool recieveOnly = true, int(*callback)(void* value) = NULL);
	int registerDataRef(XplaneTrans *NewRef, bool recieveOnly = true, int(*callback)(void* value) = NULL);
	int unRegisterDataRef(uint16_t canID);
	int setDataRefValue(uint16_t canID, float value);

protected:
	static int _processPlaneIdent(void * newInfo);

	int			_startUDP();
	int 		GetBeacon(long howLong = XP_BEACONTIMEOUT_MS);

	void 	sendUDPdata(const char *header, const byte *dataArr, const int arrSize, const int sendSize);

	static short 	 _RunState;
#define _gotPlaneId  (_RunState>=XP_RUNSTATE_GOT_PLANE)
#define _isRunnig    (_RunState>=XP_RUNSTATE_RUNNIG)

	//	bool		_isCleaning = true; // first 10 seconds of connection clean all transmisions
	long	_last_start = -10000000000; // make sure it starts first time
	long	_lastXpConnect = -10000000000;
	long	_startCleaning = -1;

	//bool _askingForPlaneId = true;

	int checkDataRefQueue();
	int checkReceiveData();

private:

	_XpCommand _findCommand(byte *buffer);
	int _ReadBeacon();
	int _processRREF(long startTs, int noBytes);

	static const int 	_UDP_MAX_PACKET_SIZE = 2048; // max size of message
	static const int 	_UDP_MAX_PACKET_SIZE_OUT = 1024; // max size of message
	static const int	_MAX_INTERVAL = 30 * 1000; 		// Max interval in ms between data messages

	byte 		_packetBuffer[_UDP_MAX_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
	byte 		_packetBufferOut[_UDP_MAX_PACKET_SIZE_OUT]; //buffer to hold incoming and outgoing packets
	WiFiUDP 	_Udp;

	//
	// Internal variables
	//
	int _LastError = XpUDP::XP_ERR_SUCCESS;
	uint16_t _XpListenPort = 49000;
	IPAddress _XPlaneIP;
	void(*_callbackFunc)(uint16_t canId, float par);
	void(*_setStateFunc)(bool state);

	//Send in a “dref_freq” of 0 to stop having X-Pane send the dataref values.
	struct _Xp_dref_struct_in {
		xint dref_freq;	//IS ACTUALLY THE NUMBER OF TIMES PER SECOND YOU WANT X-PLANE TO SEND THIS DATA!
		xint dref_en;	// reference code to pass when returning d-ref
		xchr dref_string[400];	// dataref string with optional index [xx]
	};

	_Xp_dref_struct_in _dref_struct_in;

	struct _Xp_dref_struct_out {
		xint dref_en;	//Where dref_en is the integer code you sent in for this dataref in the struct above.
		xflt dref_flt;	//Where dref_flt is the dataref value, in machine-native floating-point value, even for ints!
	};

	_Xp_dref_struct_out _dref_struct_out;

	//Use this to set ANY data-ref by UDP! With this power, you can send in any floating-point value to any data-ref in the entire sim!
	//Just look up the datarefs at http://www.xsquawkbox.net/.
	//Easy!
	//	DREF0+(4byte byte value)+dref_path+0+spaces to complete the whole message to 509 bytes

	struct _Xp_dref_struct {
		xflt var;
		xchr dref_path[500];
	};

	//SEND A -999 FOR ANY VARIABLE IN THE SELECTION LIST THAT YOU JUST WANT TO LEAVE ALONE, OR RETURN TO DEFAULT CONTROL IN THE SIMULATOR RATHER THAN UDP OVER-RIDE.

	struct _Xp_data_struct {
		uint32_t index;		 // data index, the index into the list of variables you can output from the Data Output screen in X-Plane.
		float data[8]; 	// the up to 8 numbers you see in the data output screen associated with that selection.. many outputs do not use all 8, though.
	};

	struct _Xp_becn_struct {
		char 		header[5];
		uint8_t 	beacon_major_version;		// 1 at the time of X-Plane 10.40
		uint8_t 	beacon_minor_version;		// 1 at the time of X-Plane 10.40
		xint 		application_host_id;		// 1 for X-Plane, 2 for PlaneMaker
		xint 		version_number;				// 104103 for X-Plane 10.41r3
		uint32_t	role;						// 1 for master, 2 for extern visual, 3 for IOS
		uint16_t 	port;						// port number X-Plane is listening on, 49000 by default
		xchr		computer_name[500];			// the hostname of the computer, e.g. “Joe’s Macbook”
	};

	_Xp_becn_struct _becn_struct;

	struct _DataRefs {
		uint32_t		refID;	//Internal number of dataref
		uint32_t		endRefID;	//Internal number of dataref
		uint16_t		canId;
		float			value;
		uint8_t			frequency = 5;
		bool			subscribed = false;
		bool			active = true;
		bool			isText = false;
		unsigned long	timestamp = 0;	// last read time
		bool			recieveOnly = true;
		bool			internal = false;
		int(*callback)(void* newVal);
		_Xp_dref_struct* sendStruct = NULL;
		XplaneTrans*	paramInfo = NULL;
		XpUDPdataRef*	dataRef = NULL;
	};

	//
	// a place to hold all referenced items
	//
	QList<_DataRefs *> _listRefs;
	uint32_t _lastRefId = 1;

	int _findDataRefById(int refId);
	int _getDataRefInfo(_DataRefs * newRef, XplaneTrans* thisXPinfo = NULL);
	int _registerNewDataRef(uint16_t canasId, bool recieveOnly, int(*callback)(void* value), XplaneTrans *newInfo);

	int		sendRREF(uint32_t frequency, uint32_t refID, char * xplaneId);
	int 	sendRREF(_DataRefs* newRef);

	int		sendDREF(_DataRefs* newRef);

	// queue buffers
	queueDataSetItem	_DataSetItem;
	queueDataItem		_DataItem;
};
#endif
