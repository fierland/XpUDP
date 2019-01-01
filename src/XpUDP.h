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

// define to activate code to test withiout XpLane comment after testing
//#define XpUDP_TEST_ONLY

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <QList.h>
//#include "XpUDPdataRef.h"
#include "XpUDPdataRefList.h"
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
#include "WiFiUdp.h"

constexpr auto XP_BEACONTIMEOUT_MS = 10 * 1000;
constexpr auto XP_REFQUEUEREADEY_DELAY = 100 / portTICK_PERIOD_MS;
constexpr auto XP_READER_LOOP_DELAY = 10 / portTICK_PERIOD_MS;

#define XP_Multicast_Port 49707
#define XP_MulticastAdress "239.255.1.1"

//#define XP_MulticastAdress_0 239
//#define XP_MulticastAdress_1 255
//#define XP_MulticastAdress_2 1
//#define XP_MulticastAdress_3 1

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

constexpr long _Xp_default_beacon_timeoutMs = XP_BEACONTIMEOUT_MS;
constexpr long _Xp_beacon_restart_timeoutMs = 10 * 1000;
constexpr long _Xp_default_initial_clean_timeoutMs = 10 * 1000;

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
	XP_RUNSTATE_STOPPED,
	XP_RUNSTATE_BAD_INIT,
	XP_RUNSTATE_INIT,
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
		XP_ERR_NOT_FOUND,
		XP_ERR_BAD_INITFILE,
		XP_ERR_BAD_RUNSTATE,
		XP_ERR_IS_RUNNING
	} XpErrorCode;

	//constructor
	XpUDP(const char* iniFileName);
	~XpUDP() {};
	// start connection
	int start(int toCore = 0);
	int stop();

	static short getState();

	int registerDataRef(uint16_t canID, bool recieveOnly = true, int(*callback)(void* value) = NULL);
	int registerDataRef(XplaneTrans *NewRef, bool recieveOnly = true, int(*callback)(void* value) = NULL);
	int unRegisterDataRef(uint16_t canID);
	int setDataRefValue(uint16_t canID, float value);

	int mainTaskLoop();
	int checkDataRefQueue();
	int checkReceiveData(long ts = 0);

	static int 	sendRREF(XpDataRef* newRef);

protected:
	static int	sendRREF(uint32_t frequency, uint32_t refID, char * xplaneId);

	XpUDPdataRefList _myDataRefList;

	XplaneTrans _planeInfoRec;// = { CANAS_NOD_PLANE_NAME,NULL,XP_DATATYPE_STRING,2,1,260 };
	//static XplaneTrans getPlaneName = { CANAS_NOD_PLANE_NAME,"sim/aircraft/view/acf_descrip",XP_DATATYPE_STRING,2,1,260 };
	int _unRegisterDataRefItem(uint16_t canId);

	int ParseIniFile(const char * iniFileName);
	int dataReader(long startTs);

	int checkDREFQueue();

	static int _processPlaneIdent(void * newInfo);

	int		_startUDP();
	int 	GetBeacon(long startTs, long howLong = XP_BEACONTIMEOUT_MS);

	static short 	 _RunState;
#define _gotPlaneId  (_RunState>=XP_RUNSTATE_GOT_PLANE)
#define _isRunnig    (_RunState>=XP_RUNSTATE_RUNNIG)

	//	bool		_isCleaning = true; // first 10 seconds of connection clean all transmisions
	long	_last_start = -10000000000; // make sure it starts first time
	long	_lastXpConnect = -10000000000;
	long	_startCleaning = -1;
	bool	_firstStart = true;
	int		_myCore = 0;

	//bool _askingForPlaneId = true;

	//ini file info
	int _XpIni_plane_string_len = 0;
	char _XpIni_plane_ini_file[128];
	int _XpIni_Multicast_Port;
	char _XpIni_MulticastAdress[32];
	char* _XpIniget_plane_dataref = NULL;
	long _Xp_beacon_timeoutMs = _Xp_default_beacon_timeoutMs;
	long _Xp_initial_clean_timeoutMs = _Xp_default_initial_clean_timeoutMs;

private:
	//
	// Internal variables
	//
	int _LastError = XpUDP::XP_ERR_SUCCESS;
	static uint16_t _XpListenPort;
	static IPAddress _XPlaneIP;

	TaskHandle_t xTaskReader = NULL;

	_XpCommand _findCommand(byte *buffer);
	int _ReadBeacon(long startTs);
	int _processRREF(long startTs, int noBytes);

	static const int 	_UDP_MAX_PACKET_SIZE = 2048; // max size of message
	static const int 	_UDP_MAX_PACKET_SIZE_OUT = 1500; // max size of message
	static const int	_MAX_INTERVAL = 30 * 1000; 		// Max interval in ms between data messages

	byte 		_packetBuffer[_UDP_MAX_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

	static byte 		_packetBufferOut[_UDP_MAX_PACKET_SIZE_OUT]; //buffer to hold incoming and outgoing packets
	// TODO: Check for ESP32_wifiudp ?
	static WiFiUDP 	_Udp;

	//void(*_callbackFunc)(uint16_t canId, float par);
	//void(*_setStateFunc)(bool state);

	//Send in a “dref_freq” of 0 to stop having X-Pane send the dataref values.
	struct _Xp_dref_struct_in {
		xint dref_freq;	//IS ACTUALLY THE NUMBER OF TIMES PER SECOND YOU WANT X-PLANE TO SEND THIS DATA!
		xint dref_en;	// reference code to pass when returning d-ref
		xchr dref_string[400];	// dataref string with optional index [xx]
	};

	static _Xp_dref_struct_in _dref_struct_in;

	struct _Xp_dref_struct_out {
		xint dref_en;	//Where dref_en is the integer code you sent in for this dataref in the struct above.
		xflt dref_flt;	//Where dref_flt is the dataref value, in machine-native floating-point value, even for ints!
	};

	_Xp_dref_struct_out _dref_struct_out;

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

	//int _getDataRefInfo(_DataRefs * newRef, XplaneTrans* thisXPinfo = NULL);
	int _registerNewDataRef(uint16_t canasId, bool recieveOnly, int(*callback)(void* value), XplaneTrans *newInfo);

	int		sendDREF(XpDataRef* newRef);
	static void 	sendUDPdata(const char *header, const byte *dataArr, const int arrSize, const int sendSize);

	// queue buffers
	queueDataSetItem	_DataSetItem;
	queueDataItem		_DataItem;
};
#endif
