//==================================================================================================
//  Franks Flightsim Intruments project
//  by Frank van Ierland
//
// This code is in the public domain.
//
//==================================================================================================

//==================================================================================================
// XpUDP.h
//
// author: Frank van Ierland
// version: 0.1
//
// UDP communication to Xplane server
//
//------------------------------------------------------------------------------------
/// \author  Frank van Ierland (frank@van-ierland.com) DO NOT CONTACT THE AUTHOR DIRECTLY: USE THE LISTS
// Copyright (C) 2018 Frank van Ierland// setting up the UPD stuff
//==================================================================================================
// TODO: add fixed ethernet capability
//==================================================================================================
#define LOG_LOCAL_LEVEL 5
static const char *TAG = "XpUDP";

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "XpUDP.h"
#include "xpudp_debug.h"
#include "XpPlaneInfo.h"

#include <IniFile.h>

//==================================================================================================
// function to get info from ini file
//[xplane]
//get_plane = "sim/aircraft/view/acf_descrip"
//get_plane_size = 260
//
//udp_ip = 239.255.1.1
//upd_port = 49707
//
//beacon_timeout = 10000
//clean_timeout = 10000
//--------------------------------------------------------------------------------------------------

int XpUDP::ParseIniFile(const char* iniFileName)
{
	const size_t bufferLen = 160;
	char buffer[bufferLen];
	int result = 0;
	int iVal;
	unsigned long lVal;

	ESP_LOGD(TAG, "start parse ini :%s", iniFileName);

	IniFile ini(iniFileName, "rb");

	delay(10);

	if (!ini.open())
	{
		ESP_LOGE(TAG, "Ini file does not exist");
		// Cannot do anything else
		return -1;
	}

	ESP_LOGV(TAG, "Inifile opened");

	// Fetch a value from a key which is present
	if (ini.getValue("xplane", "get_plane_info", buffer, bufferLen))
	{
		strcpy(_XpIni_plane_ini_file, buffer);
		ESP_LOGD(TAG, "Set plane file to= %s", _XpIni_plane_ini_file);
	}
	else
	{
		//printErrorMessage(ini.getError());
		ESP_LOGE(TAG, "Can not get info from file. Err= %d", ini.getError());
		result = -1;
	}

	if (ini.getValue("xplane", "get_plane_size", buffer, bufferLen, iVal))
	{
		ESP_LOGD(TAG, "Set plane string size to= %d", _XpIni_plane_ini_file);
		_XpIni_plane_string_len = iVal;
	}
	else
	{
		//printErrorMessage(ini.getError());
		ESP_LOGE(TAG, "Can not get info from file. Err= %d", ini.getError());
		result = -1;
	}

	if (ini.getValue("xplane", "get_plane_dataref", buffer, bufferLen))
	{
		ESP_LOGD(TAG, "set _XpIniget_plane_dataref to= %s", buffer);
		_XpIniget_plane_dataref = (char*)malloc(strlen(buffer) + 1);
		strcpy(_XpIniget_plane_dataref, buffer);
	}

	if (ini.getValue("xplane", "udp_ip", buffer, bufferLen))
	{
		ESP_LOGD(TAG, "set ip to=%s:", buffer);
		strcpy(_XpIni_MulticastAdress, buffer);
	}
	else
	{
		ESP_LOGE(TAG, "Can not get info from file. Err= %d", ini.getError());
		result = -1;
	}

	if (ini.getValue("xplane", "upd_port", buffer, bufferLen, iVal))
	{
		ESP_LOGD(TAG, "set port to= %d", iVal);
		_XpIni_Multicast_Port = iVal;
	}
	else
	{
		//printErrorMessage(ini.getError());
		ESP_LOGE(TAG, "Can not get info from file. Err= %d", ini.getError());
		result = -1;
	}

	if (ini.getValue("xplane", "beacon_timeout", buffer, bufferLen, lVal))
	{
		ESP_LOGD(TAG, "set _Xp_beacon_timeoutMs to= %d", lVal);
		_Xp_beacon_timeoutMs = lVal;
	}

	if (ini.getValue("xplane", "clean_timeout", buffer, bufferLen, lVal))
	{
		ESP_LOGD(TAG, "set _Xp_initial_clean_timeoutMs to= %d", lVal);
		_Xp_initial_clean_timeoutMs = lVal;
	}

	ini.close();

	// pass filename to XpPlaneInfoClass.
	XpPlaneInfo::setIniFile(_XpIni_plane_ini_file);

	ESP_LOGV(TAG, "Inifile done no errors");
	return 0;
}

//==================================================================================================
//--------------------------------------------------------------------------------------------------
extern "C" void taskXplane(void* parameter)
{
	XpUDP* xpudp;

	ESP_LOGD(TAG, "Task taskXplane started ");

	xpudp = (XpUDP*)parameter;
	for (;;)
	{
		esp_task_wdt_reset();
		xpudp->mainTaskLoop();
		vTaskDelay(XP_READER_LOOP_DELAY);
	}
}
//==================================================================================================
//-------------------------------------------------------------------------------------------------
extern "C" void taskXplaneRefQueueReader(void* parameter)
{
	XpUDP* xpudp;
	ESP_LOGD(TAG, "Task taskXplaneRefQueueReader started ");

	xpudp = (XpUDP*)parameter;
	for (;;)
	{
		esp_task_wdt_reset();
		xpudp->checkDataRefQueue();
		vTaskDelay(XP_REFQUEUEREADEY_DELAY);
	}
}
//==================================================================================================
//-------------------------------------------------------------------------------------------------
extern "C" void taskXplaneCheckReceive(void* parameter)
{
	XpUDP* xpudp;
	ESP_LOGD(TAG, "Task taskXplaneCheckReceive started ");

	xpudp = (XpUDP*)parameter;
	for (;;)
	{
		esp_task_wdt_reset();
		xpudp->checkReceiveData();
		vTaskDelay(XP_REFQUEUEREADEY_DELAY);
	}
}

//==================================================================================================
// static vars
//--------------------------------------------------------------------------------------------------
//bool	XpUDP::_gotPlaneId = false;
short 	 XpUDP::_RunState = XP_RUNSTATE_INIT;

// inter task stuff
SemaphoreHandle_t xSemaphoreRefs = NULL;

//==================================================================================================
//	constructor
//  TODO: remove callbacks
//--------------------------------------------------------------------------------------------------
XpUDP::XpUDP(const char* iniFileName)
{
	DLPRINTINFO(2, "START");

	DLPRINTLN(1, "set state to XP_RUNSTATE_STOP");
	_RunState = XP_RUNSTATE_INIT;
	ESP_LOGV(TAG, "Runstate is XP_RUNSTATE_INIT\n");

	vSemaphoreCreateBinary(xSemaphoreRefs);

	if (ParseIniFile(iniFileName) != 0)
	{
		ESP_LOGE(TAG, "initfile not correct");
		_RunState = XP_RUNSTATE_BAD_INIT;
	};

	DLPRINTINFO(2, "STOP");
}

//==================================================================================================
//	start the session with the X-Plane server
// TODO: check where used
//==================================================================================================
int XpUDP::start(int toCore)
{
	int 	result;

	// check if bus is already running
	//DLPRINTINFO(2, "START");

	if (_RunState == XP_RUNSTATE_BAD_INIT)
	{
		ESP_LOGE(TAG, "!!STOPPING: Bad initfile can not start");
		return -XP_ERR_BAD_INITFILE;
	}

	if (!_RunState == XP_RUNSTATE_STOPPED)
	{
		if (_RunState >= XP_RUNSTATE_GOT_BEACON)
		{
			ESP_LOGE(TAG, "!!Bad start: Is already running");
			return -3;
		}

		ESP_LOGV(TAG, "Starting UDP func");
		//if (_startUDP()) GetBeacon();
	}

	_myCore = toCore;

	// starting main tasks
	ESP_LOGV(TAG, "Startting main tasks");

	xTaskCreatePinnedToCore(taskXplane, "taskXPlane", 15000, this, 4, &xTaskReader, _myCore);
	xTaskCreatePinnedToCore(taskXplaneRefQueueReader, "taskXPlaneRefQueue", 10000, this, 2, &xTaskRefQueue, _myCore);

	esp_task_wdt_add(xTaskReader);
	esp_task_wdt_add(xTaskRefQueue);

	if (_RunState == XP_RUNSTATE_STOPPED)
	{
		_RunState = XP_RUNSTATE_RUNNIG;
		xEventGroupSetBits(s_connection_event_group, RUNNING_CONNECTED_BIT);
		ESP_LOGV(TAG, "Runstate is XP_RUNSTATE_RUNNIG\n");
	}

	//DLPRINTINFO(2, "STOP");
	return result;
}
//==================================================================================================
//
//==================================================================================================
int XpUDP::stop()
{
	vTaskDelete(xTaskRefQueue);
	vTaskDelete(xTaskReader);

	xEventGroupClearBits(s_connection_event_group, RUNNING_CONNECTED_BIT);
	_RunState = XP_RUNSTATE_STOPPED;
	ESP_LOGV(TAG, "Runstate is XP_RUNSTATE_STOPPED");

	return 0;
}
//==================================================================================================
//
//==================================================================================================
short XpUDP::getState()
{
	//DLVARPRINTLN(1, "RunState:", _RunState);
	return _RunState;
}
//==================================================================================================
// main loop
//==================================================================================================
int XpUDP::mainTaskLoop()
{
	long startTs = millis();

	esp_task_wdt_reset();
	switch (_RunState)
	{
	case XP_RUNSTATE_INIT:
		if (xEventGroupWaitBits(s_connection_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, 100) != WIFI_CONNECTED_BIT)
		{
			return -XP_ERR_FAILED;
		}
		_RunState = XP_RUNSTATE_WIFI_UP;
		ESP_LOGV(TAG, "Runstate is XP_RUNSTATE_WIFI_UP");
	case XP_RUNSTATE_WIFI_UP:
		if (_startUDP() == XP_ERR_SUCCESS)
		{
			ESP_LOGV(TAG, "Set state to XP_RUNSTATE_UDP_UP");
			_RunState = XP_RUNSTATE_UDP_UP;
		}
		else break;
	case XP_RUNSTATE_UDP_UP:
		//ESP_LOGV(TAG, "check for beacon start=%d last=%d interval=%d", startTs, _last_start, _Xp_beacon_restart_timeoutMs);
		while (_Xp_beacon_restart_timeoutMs > (millis() - _last_start));
		//ESP_LOGV(TAG, "done wait");
		if (GetBeacon() == XP_ERR_SUCCESS)
		{
			ESP_LOGV(TAG, "Set state to XP_RUNSTATE_GOT_BEACON");
			_RunState = XP_RUNSTATE_GOT_BEACON;
		}
		else break;
	case XP_RUNSTATE_GOT_BEACON:
		if (_firstStart)
		{
			_firstStart = false;
			_RunState = XP_RUNSTATE_IS_CLEANING;
			ESP_LOGV(TAG, "Set state to XP_RUNSTATE_IS_CLEANING");
		}
		else
		{
			_RunState = XP_RUNSTATE_DONE_CLEANING;
			ESP_LOGV(TAG, "Set state to XP_RUNSTATE_DONE_CLEANING");
		}
		break;
	case XP_RUNSTATE_IS_CLEANING:
		if (_Xp_initial_clean_timeoutMs < (startTs - _startCleaning))
		{
			DLPRINTLN(2, "End cleaning");
			xEventGroupClearBits(s_connection_event_group, CLEANING_CONNECTED_BIT);
			_RunState = XP_RUNSTATE_DONE_CLEANING;
			ESP_LOGV(TAG, "Runstate is XP_RUNSTATE_DONE_CLEANING");
			// ask for plane id
			_planeInfoRec.canasId = CANAS_NOD_PLANE_NAME;
			_planeInfoRec.xplaneId = _XpIniget_plane_dataref;
			registerDataRef(&_planeInfoRec, true, _processPlaneIdent);
		}
		dataReader(startTs);
		break;
	case XP_RUNSTATE_DONE_CLEANING:
		// check UDP as long as we did not receive a valid plane name
		dataReader(startTs);
		break;
	case XP_RUNSTATE_GOT_PLANE:
		unRegisterDataRef(_planeInfoRec.canasId);
		//_setStateFunc(true);  // signal to canbus interface that xplane interface is running
		xEventGroupSetBits(s_connection_event_group, PLANE_CONNECTED_BIT | RUNNING_CONNECTED_BIT);
		_RunState = XP_RUNSTATE_RUNNIG;
		ESP_LOGV(TAG, "Runstate is XP_RUNSTATE_RUNNING");
		if (xTaskCheckReceive == NULL)
			xTaskCreatePinnedToCore(taskXplaneCheckReceive, "taskXplaneCheckReceive", 1000, this, 1, &xTaskCheckReceive, _myCore);
		break;
	case XP_RUNSTATE_RUNNIG:
		// check if we are still reiving data from XPlane
		if (_Xp_beacon_timeoutMs < (startTs - _lastXpConnect))
		{
			ESP_LOGV(TAG, "Runstate is XP_RUNSTATE_UDP_UP");
			xEventGroupClearBits(s_connection_event_group, BEACON_CONNECTED_BIT | PLANE_CONNECTED_BIT);
			_RunState = XP_RUNSTATE_UDP_UP;
		}
		dataReader(startTs);
		// only if we are fully running
		checkDREFQueue();

		break;
	case XP_RUNSTATE_STOPPED:
		// do noting util bus is restarted
		break;
	default:
		ESP_LOGE(TAG, "Bad runstate :%d", _RunState);
		break;
	}
}
//==================================================================================================
//	data reader for in loop() pols the udp port for new messages and dispaces them to controls
// TODO: process arrays
// TODO: create task
//==================================================================================================
int XpUDP::dataReader(long startTs)
{
	int message_size = 0;
	int noBytes = 0;
	_DataRefs* tmpRef;

	_LastError = XpUDP::XP_ERR_SUCCESS;

	// check for UDP input
	noBytes = _Udp.parsePacket(); //// returns the size of the packet

	if (noBytes > 0)
	{
		DLVARPRINTLN(5, "Packet found bytes= ", noBytes);
		message_size = _Udp.read(_packetBuffer, noBytes); // read the packet into the buffer
		DLVARPRINT(5, ":Packet of ", noBytes);
		_lastXpConnect = millis();

		_XpCommand thisCommand = _findCommand(_packetBuffer);

		if (thisCommand != XP_CMD_BECN)    // skip beacon pakages
		{
			DLPRINT(5, millis() / 1000);

			DLVARPRINT(5, " received from ", _Udp.remoteIP());
			DLPRINTLN(5, _Udp.remotePort());
			DLVARPRINT(5, "Header:", (char *)_packetBuffer);
			DLPRINTBUFFER(5, _packetBuffer, noBytes);
		}

		// check if header is RREF
		switch (thisCommand)
		{
		case XP_CMD_RREF:
			DLPRINT(5, ":start RREF");
			_processRREF(startTs, noBytes);
			break;
		case XP_CMD_BECN:
			_lastXpConnect = millis();
			break;
		default:
			DLPRINTLN(0, "!!Unexpexted datatype encountered:");
			DLVARPRINTLN(0, " code:", (int)thisCommand);
			_LastError = XpUDP::XP_ERR_NO_DATA;
			// read the values into array
		}
	}

	DLPRINTINFO(5, "STOP");
	return _LastError;
}
//==================================================================================================
// process new plane identification string
//==================================================================================================
int XpUDP::_processPlaneIdent(void* newInfo)
{
	DLPRINTINFO(2, "START");

	char* newPlane = (char*)newInfo;

	ESP_LOGI(TAG, "Testing:%s:", newPlane);

	if (XpPlaneInfo::findPlane(newPlane) == 0)
	{
		ESP_LOGI(TAG, "Set state to XP_RUNSTATE_GOT_PLANE");
		_RunState = XP_RUNSTATE_GOT_PLANE;
		xEventGroupSetBits(s_connection_event_group, PLANE_CONNECTED_BIT);
		return 0;
	};

	DLPRINTINFO(2, "STOP");
	return -1;
}

//==================================================================================================
//	get beacon from X Plane server and store adres and port
//==================================================================================================
int XpUDP::_startUDP()
{
	IPAddress   XpMulticastAdress;
	uint16_t  	XpMulticastPort = _XpIni_Multicast_Port;

	assert(XpMulticastAdress.fromString(_XpIni_MulticastAdress) == true);

	//assert(_XpIni_MulticastAdress.isValid() == true);

	//Serial.print("Ip adres=");
	//Serial.println(XpMulticastAdress);

	if (_RunState > XP_RUNSTATE_UDP_UP)
	{
		ESP_LOGE(TAG, "bad call to _startUDP, wrong state %d", _RunState);
		return -XP_ERR_FAILED;
	}
	_last_start = millis();

	_LastError = XpUDP::XP_ERR_SUCCESS;

	// start connection
	ESP_LOGV(TAG, "Starting connection, listening on: %s port: %d", _XpIni_MulticastAdress, _XpIni_Multicast_Port);

	if (_Udp.beginMulticast(XpMulticastAdress, XpMulticastPort))
	{
		xEventGroupSetBits(s_connection_event_group, UDP_CONNECTED_BIT);
		ESP_LOGV(TAG, "Udp start ok");
	}
	else
	{
		ESP_LOGE(TAG, "!!Udp start failed");
		_LastError = -XP_ERR_FAILED;
	}

	return _LastError;
};
//==================================================================================================
//	get beacon from X Plane server and store adres and port
//==================================================================================================
int XpUDP::GetBeacon(long howLong)
{
	DLPRINTINFO(5, "START");
	long startTs = millis();
	int result = -XP_ERR_FAILED;

	if (_RunState < XP_RUNSTATE_UDP_UP)
	{
		ESP_LOGE(TAG, "Bad Runstate: %d\n", _RunState);
		return -XP_ERR_BAD_RUNSTATE;
	}

	_LastError = XpUDP::XP_ERR_SUCCESS;

	// wait for beacon transmision
	do
	{
		result = _ReadBeacon();
		esp_task_wdt_reset();
		delay(10);
	} while (result != 0 && ((millis() - startTs) < howLong));

	return result;
	DLPRINTINFO(5, "Stop");
}
//------------------------------------------------------------------------------------------------------------
int XpUDP::_ReadBeacon()
{
	int message_size;
	int noBytes;
	long startTs = millis();
	int result = XpUDP::XP_ERR_SUCCESS;

	_LastError = XpUDP::XP_ERR_SUCCESS;

	ESP_LOGV(TAG, "Search for beacon");

	noBytes = _Udp.parsePacket(); //// returns the size of the packet

	if (noBytes > 0)
	{
		ESP_LOGV(TAG, "Packet of %d received", noBytes);

		_XPlaneIP = _Udp.remoteIP();

		if (noBytes < _UDP_MAX_PACKET_SIZE)
		{
			message_size = _Udp.read(_packetBuffer, noBytes); // read the packet into the buffer

			if (message_size < sizeof(_becn_struct))
			{
				// copy to breacon struct and compensate byte alignment at 8th byte

				memcpy(&_becn_struct, _packetBuffer, 8);
				memcpy(&((char*)&_becn_struct)[8], (_packetBuffer)+7, sizeof(_becn_struct) - 8);
				;

				if (strcmp(_becn_struct.header, XPMSG_BECN) == 0)
				{
					ESP_LOGD(TAG, "UDP beacon received");
					ESP_LOGD(TAG, "computer_name      :%s role:%d", _becn_struct.computer_name, _becn_struct.role);
					ESP_LOGD(TAG, "beacon_version     :%d.%d", _becn_struct.beacon_major_version, _becn_struct.beacon_minor_version);
					ESP_LOGD(TAG, "application_host_id:%d", _becn_struct.application_host_id);
					ESP_LOGD(TAG, "version_number     :%d", _becn_struct.version_number);
					ESP_LOGD(TAG, "application_host_id:%d", _becn_struct.application_host_id);
					ESP_LOGD(TAG, "port               : %d", _becn_struct.port);

					if (_becn_struct.beacon_major_version != 1 || _becn_struct.beacon_minor_version != 2)
					{
						ESP_LOGE(TAG, "!!Beacon: Version wrong version found: %d.%d",
							_becn_struct.beacon_major_version, _becn_struct.beacon_minor_version);

						_LastError = XpUDP::XP_ERR_WRONG_HEADER;
					}
					// store listen port
					_XpListenPort = _becn_struct.port;

					// correct start so update status
					_lastXpConnect = millis();
					ESP_LOGD(TAG, "Set state to XP_RUNSTATE_IS_CLEANING");
					_RunState = XP_RUNSTATE_IS_CLEANING;
					xEventGroupSetBits(s_connection_event_group, CLEANING_CONNECTED_BIT);
					_startCleaning = _lastXpConnect;

					// inform CANBUS Manager that system is running
					//if (_setStateFunc != NULL)
					//	_setStateFunc(_RunState);
				}
				else
				{
					ESP_LOGE(TAG, "!!Beacon: Header wrong", _becn_struct.header);

					_LastError = XpUDP::XP_ERR_WRONG_HEADER;
				}
			}
			else
			{
				ESP_LOGE(TAG, "!!Beacon: Message size to big for struct");

				_LastError = XpUDP::XP_ERR_BUFFER_OVERFLOW;
			}
		}
		else
		{
			ESP_LOGE(TAG, "!!Beacon: Message size to big for buffer");

			_LastError = XpUDP::XP_ERR_BUFFER_OVERFLOW;
		}
	}
	else
	{
		ESP_LOGI(TAG, "Beacon: Nothing to read, beacon not found");
		_LastError = XpUDP::XP_ERR_BEACON_NOT_FOUND;
	}
	// everything ok :-)

	// DLPRINTINFO(5, "STOP");
	//return 0;

	return _LastError;
}

//==================================================================================================
// parse buffer for command string
//==================================================================================================
_XpCommand XpUDP::_findCommand(byte *buffer)
{
	int i = 0;
	int j = 0;
	DLPRINTINFO(4, "START");
	// check first char
	DLVARPRINTLN(5, "First char=", _packetBuffer[0]);

	while ((_myListCommands[i].text[0] != 'X') && (_packetBuffer[0] > _myListCommands[i].text[0])) i++;

	if (_myListCommands[i].text[0] == _packetBuffer[0])
	{
		DLVARPRINTLN(5, "2nd char=", _packetBuffer[1]);
		// check 2nd char
		j = i;
		while ((_myListCommands[j].text[0] == _packetBuffer[0]) && (_packetBuffer[1] != _myListCommands[j].text[1])) j++;

		if ((_packetBuffer[0] == _myListCommands[j].text[0]) && (_packetBuffer[1] == _myListCommands[j].text[1]))
		{
			DLVARPRINTLN(4, "returning", _myListCommands[j].code);
			return _myListCommands[j].code;
		}
	}
	DLPRINT(1, "!!No match");
	DLPRINTINFO(1, "STOP");
	return XP_CMD_XXXX;
}
//==================================================================================================
//==================================================================================================
int XpUDP::_findDataRefById(int refId)
{
	_DataRefs *tmpRef;
	DLPRINTINFO(4, "START");

	DLVARPRINTLN(4, "items in list:", _listRefs.size());

	for (int i = 0; i < _listRefs.size(); i++)
	{
		tmpRef = _listRefs[i];
		if ((tmpRef->refID <= refId) && (tmpRef->endRefID >= refId))
		{
			return i;
			break;
		}
	}
	return -XP_ERR_NOT_FOUND;
	DLPRINTINFO(4, "STOP");
}
int XpUDP::_findDataRefByCanId(uint16_t CanId)
{
	_DataRefs *tmpRef;
	DLPRINTINFO(4, "START");

	DLVARPRINTLN(4, "items in list:", _listRefs.size());

	for (int i = 0; i < _listRefs.size(); i++)
	{
		tmpRef = _listRefs[i];
		if (tmpRef->canId == CanId)
		{
			return i;
			break;
		}
	}
	return -XP_ERR_NOT_FOUND;
	DLPRINTINFO(4, "STOP");
}
;
//==================================================================================================
// proccess a RREF command and send uopdated values to CANBUS manager
// TODO: parse string if needed
// TODO: special action on certain info records
//==================================================================================================
int XpUDP::_processRREF(long startTs, int noBytes)
{
	int curRecord = -1;
	_DataRefs* tmpRef;
	_DataRefs* curRef = NULL;

	DLPRINTLN(5, "Start processing RREF");
	delay(10);

	for (int idx = 5; idx < noBytes; idx += 8)
	{
		memcpy(&_dref_struct_out, _packetBuffer + idx, sizeof(_dref_struct_out));

		esp_task_wdt_reset();

		DLVARPRINT(5, "idx=", idx);
		DLVARPRINTLN(5, ":at idx=", _packetBuffer[idx], HEX);
		//DLVARPRINTLN(1, "struct size=", sizeof(_dref_struct_out));
		DLVARPRINT(5, "XPlane Received (udp)    RREF=", _dref_struct_out.dref_en);
		DLVARPRINTLN(5, " value=", _dref_struct_out.dref_flt);

		float value = xflt2float(_dref_struct_out.dref_flt);
		uint32_t en = xint2uint(_dref_struct_out.dref_en);

		ESP_LOGV(TAG, "XPlane Received (native) RREF=%d value=%d (%c)", en, value, value);

		if (en != 0)
		{
			DLPRINT(5, "Ask for semaphore");
			while (xSemaphoreTake(xSemaphoreRefs, (TickType_t)10) != pdTRUE); // wait for semaphore
			DLPRINT(5, "Got semaphore");

			// check if we know the dataref number
			int i = _findDataRefById(en);

			// process data
			if (i > -1 && _listRefs[i]->subscribed && _RunState >= XP_RUNSTATE_DONE_CLEANING)
			{
				curRef = _listRefs[i];

				assert(curRef->dataRef != NULL);

				if (!curRef->isText)
				{
					curRef->timestamp = millis();
					DLPRINT(5, "not text");

					if (true) //(curRef->value != value)
					{
						ESP_LOGD(TAG, "XPlane Updating value of %d from %d to %d", curRef->canId, curRef->dataRef->getValue(), value);

						// TODO: Call to update function
						// canid's < 200 are usd for info only relevant to XpUDP interface like plane name
						if (curRef->paramInfo->canasId > 200)
						{
							_DataItem.canId = curRef->paramInfo->canasId;
							_DataItem.value = value;
							if (xQueueSendToBack(xQueueRREF, &_DataItem, 10) != pdPASS)
							{
								ESP_LOGE(TAG, "Error posting RREF to queue");
							}
						}
					}
					curRef->dataRef->setValue(value);
				}
				else
				{
					DLPRINT(4, "is text");

					((XpUDPstringDataRef*)curRef->dataRef)->setValue(value, (en - curRef->refID));

					if (curRef->dataRef->isChanged())
					{
						curRef->timestamp = millis();
						ESP_LOGD(TAG, "String done");
						// TODO:if changed
					}
					//ESP_LOGD(TAG, "XPlane Updating string value of %d to :%s:", curRef->canId, ((XpUDPstringDataRef*)curRef->dataRef)->getCurrentStrValue());
				}
			}
			else
			{
				// not found in subscription list so sign out of a RREF
				ESP_LOGD(TAG, "!!Code not found in subscription list or cleaning up, signing out of RREF:%d", en);
				sendRREF(0, en, "");
				//if (_RunState = XP_RUNSTATE_IS_CLEANING) _startCleaning = millis();
			}

			DLPRINT(5, "Release semaphore");
			xSemaphoreGive(xSemaphoreRefs); // release semaphore
		}
	}
}
//==================================================================================================
// TODO: create task :-)
//==================================================================================================

int XpUDP::checkDREFQueue()
{
	int curRec;

	while (xQueueReceive(xQueueDREF, &_DataItem, 0) == pdTRUE)
	{
		if (xSemaphoreTake(xSemaphoreRefs, (TickType_t)10) == pdTRUE)
		{
			curRec = _findDataRefByCanId(_DataItem.canId);
			if (curRec != -XP_ERR_NOT_FOUND)
			{
				_listRefs[curRec]->value = _DataItem.value;
				_listRefs[curRec]->timestamp = millis();
				sendDREF(_listRefs[curRec]);
			}
			xSemaphoreGive(xSemaphoreRefs);
		}
	};

	return 0;
};

//==================================================================================================
// TODO: create task :-)
//==================================================================================================

int XpUDP::checkDataRefQueue()
{
	while (xQueueReceive(xQueueDataSet, &_DataSetItem, 0) == pdTRUE)
	{
		if (_DataSetItem.interval == 0)
			unRegisterDataRef(_DataSetItem.canId);
		else
			registerDataRef(_DataSetItem.canId, _DataSetItem.send);
	};

	return 0;
};

//==================================================================================================
	// now loop to check if we are receiving all data
	// TODO: make task
//--------------------------------------------------------------------------------------------------
int XpUDP::checkReceiveData()
{
	_DataRefs* tmpRef;

	DLPRINT(6, "check receive loop");

	if (_RunState >= XP_RUNSTATE_DONE_CLEANING)
	{
		unsigned long timeNow = millis();
		if (xSemaphoreTake(xSemaphoreRefs, (TickType_t)10) == pdTRUE)
		{
			for (int i = 0; i < _listRefs.size(); i++)
			{
				tmpRef = _listRefs[i];

				// check if we have a info record
				if (tmpRef->paramInfo == NULL)
				{
					_getDataRefInfo(tmpRef);
					ESP_LOGD(TAG, "Stack high wateer =%d", uxTaskGetStackHighWaterMark(NULL));
				}

				if (tmpRef->active && (tmpRef->paramInfo != NULL) && ((timeNow - tmpRef->dataRef->lastTs()) > _MAX_INTERVAL))
				{
					ESP_LOGD(TAG, "!!Item not receiving data (resend RREF):%d interval=%d now=%d ts=%d",
						i, _MAX_INTERVAL, timeNow, tmpRef->timestamp);

					sendRREF(tmpRef);
					tmpRef->timestamp = timeNow;
					break;
				}
			}
			xSemaphoreGive(xSemaphoreRefs);
		}
	}
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

//==================================================================================================
//==================================================================================================
int XpUDP::_getDataRefInfo(_DataRefs* newRef, XplaneTrans* thisXPinfo)
{
	DLPRINTINFO(4, "START");

	if (newRef->paramInfo == NULL)
	{
		XplaneTrans* curXPinfo = NULL;
		// find record
		if (thisXPinfo == NULL)
		{
			if (_isRunnig) 	curXPinfo = XpPlaneInfo::fromCan2XplaneElement(newRef->canId);
		}
		else
			curXPinfo = thisXPinfo;

		newRef->paramInfo = curXPinfo;
	}

	if (newRef->paramInfo != NULL)
	{
		newRef->frequency = newRef->paramInfo->xpTimesSecond;
		if (newRef->paramInfo->xpDataType == XP_DATATYPE_STRING)
		{
			newRef->endRefID += newRef->paramInfo->stringSize;
			_lastRefId = newRef->endRefID + 1;
			newRef->isText = true;
		}
		DLVARPRINT(1, "startRef:", newRef->refID);
		DLVARPRINTLN(1, ":endRef:", newRef->endRefID);

		if (newRef->dataRef == NULL)
		{
			ESP_LOGD(TAG, "Adding new dataref for %d", newRef->canId);

			if (newRef->paramInfo->xpDataType == XP_DATATYPE_STRING)
			{
				DLPRINTLN(1, "adding string");
				newRef->dataRef = new XpUDPstringDataRef(newRef->refID, newRef->paramInfo->stringSize);
			}
			else
			{
				DLPRINTLN(1, "adding var");
				newRef->dataRef = new XpUDPfloatDataRef(newRef->refID);
			}
			if (newRef->callback != NULL)
				newRef->dataRef->setCallback(newRef->callback);
		}

		if (!newRef->recieveOnly)
		{
			newRef->sendStruct = (_Xp_dref_struct*)malloc(sizeof(_Xp_dref_struct));
			strcpy(newRef->sendStruct->dref_path, newRef->paramInfo->xplaneId);
		}
	}
	else
	{
		ESP_LOGE(TAG, "STOP no dataref info found for %d", newRef->canId);
		return -1;
	}

	DLPRINTINFO(4, "STOP");
	return 0;
}
//==================================================================================================
int XpUDP::_registerNewDataRef(uint16_t canasId, bool recieveOnly, int(*callback)(void* value), XplaneTrans *newInfo)
{
	int curRecord = -1;
	_DataRefs* newRef = NULL;

	DLPRINTINFO(4, "START");

	// check if struct is free to manipulate
	while (xSemaphoreTake(xSemaphoreRefs, (TickType_t)10) != pdTRUE);

	for (int i = 0; i < _listRefs.size(); i++)
	{
		newRef = _listRefs[i];
		if (newRef->canId == canasId)
		{
			curRecord = i;
			break;
		}
	}

	if (curRecord == -1)  // not found in list
	{
		//not found so create new item
		ESP_LOGV(TAG, "Registering dataref: %d", canasId);

		newRef = new _DataRefs;

		_listRefs.push_back(newRef);
		newRef->refID = _lastRefId++;
		newRef->endRefID = newRef->refID;
		newRef->canId = canasId;
		newRef->paramInfo = newInfo;
	}
	else
	{
		// found a record so update item
		DLPRINT(1, "existing reccord");
		newRef = _listRefs[curRecord];
	}

	newRef->active = true;

	newRef->recieveOnly = recieveOnly;
	if (callback != NULL)
		newRef->callback = callback;

	DLVARPRINTLN(4, "infoRecord=", (newInfo == NULL) ? 0 : 1);

	_getDataRefInfo(newRef);

	if (newRef->dataRef == NULL) DLPRINTLN(2, "NO DATAREF");

	// check if active and for last timestamp
	if (_RunState >= XP_RUNSTATE_DONE_CLEANING && (newRef->paramInfo != NULL) && (newRef->recieveOnly) && (!newRef->subscribed || (millis() - newRef->timestamp) > _MAX_INTERVAL))
	{
		// create new request
		DLPRINTLN(1, "create x-plane request");

		sendRREF(newRef);

		// read first returned value
		//dataReader();
	}
	else
	{
		DLPRINTLN(4, "not subscribing");
	}

	xSemaphoreGive(xSemaphoreRefs);
	DLPRINTINFO(4, "STOP");

	return (newRef != NULL) ? XP_ERR_SUCCESS : -XP_ERR_FAILED;
}
//==================================================================================================
int XpUDP::registerDataRef(XplaneTrans *newInfo, bool recieveOnly, int(*callback)(void* value))
{
	DLPRINTINFO(2, "START by transRec");

	DLVARPRINTLN(1, "Registering:", newInfo->canasId);

	assert(_registerNewDataRef(newInfo->canasId, recieveOnly, callback, newInfo) == XP_ERR_SUCCESS);

	DLPRINTINFO(2, "STOP");

	return 0;
}
//==================================================================================================
// create new data referece link with X-Plane
//
// the CanID will be the unoique ref for all datarequests in the system
// in this struct we will only register datarefs we need from XPlane. // ??
/// canasId  : internal id for dataref
/// receiveOnly : if true we dont need to put this in the CanBus queue but will use an internal callback
/// callback : internal function to directly process the value (like for the plane name).
///==================================================================================================
int XpUDP::registerDataRef(uint16_t canasId, bool recieveOnly, int(*callback)(void* value))
{
	DLPRINTINFO(5, "START by canId");

	DLVARPRINTLN(1, "Registering:", canasId);
	// check if struct is free to manipulate

	assert(_registerNewDataRef(canasId, recieveOnly, callback, NULL) == XP_ERR_SUCCESS);

	DLPRINTINFO(5, "STOP");

	return 0;
}

//==================================================================================================
//--------------------------------------------------------------------------------------------------
int XpUDP::unRegisterDataRef(uint16_t canID)
{
	bool notFound = true;
	int curRecord = -1;
	_DataRefs* tmpRef;

	DLPRINTINFO(2, "START");
	DLVARPRINTLN(1, "Unregister request for:", canID);

	while (xSemaphoreTake(xSemaphoreRefs, (TickType_t)10) != pdTRUE);

	DLVARPRINTLN(1, "checking #items:", _listRefs.size());

	// first test if we have the item.
	for (int i = 0; notFound && (i < _listRefs.size()); i++)
	{
		tmpRef = _listRefs[i];

		assert(tmpRef != NULL);

		DLVARPRINTLN(2, "check:", tmpRef->canId);

		if (tmpRef->canId == canID)
		{
			DLVARPRINTLN(2, "found:", i);

			notFound = false;
			curRecord = i;
			tmpRef->active = false;
			tmpRef->subscribed = false;
			tmpRef->frequency = (uint8_t)0;
			ESP_LOGV(TAG, "Unregister dataref: %d", tmpRef->canId);

			if (tmpRef->isText)
			{
				for (int j = tmpRef->refID; j <= tmpRef->endRefID; j++)
					sendRREF(0, j, "");
			}
			else
				sendRREF(tmpRef);
			DLPRINTLN(1, "removing datarefs");
			if (tmpRef->dataRef != NULL)
			{
				delete tmpRef->dataRef;
				tmpRef->dataRef = NULL;
			}

			if (tmpRef->sendStruct != NULL) free(tmpRef->sendStruct);

			break;
		}
		else
		{
			DLVARPRINTLN(2, "Skip:", i);
		}
	}
	xSemaphoreGive(xSemaphoreRefs);
	DLPRINTINFO(2, "STOP");
	return 0;
}
///==================================================================================================
// send a updat to a xref to x-plane
///==================================================================================================
int XpUDP::setDataRefValue(uint16_t canID, float value)
{
	// TODO: Code send data to Xplane
	_DataRefs* tmpRef;
	int curRecord = -1;
	DLPRINTINFO(2, "START");

	// first test if we have the item.
	for (int i = 0; i < _listRefs.size(); i++)
	{
		tmpRef = _listRefs[i];
		if (tmpRef->canId == canID)
		{
			curRecord = i;
			break;
		}
	}

	if (curRecord != -1)
	{
		tmpRef->value = value;
		DLVARPRINT(1, "Send update Data ref", canID); DLVARPRINTLN(1, ":", value);
		sendDREF(tmpRef);
	}
	else
	{
		DLPRINTLN(0, "!! Data ref not found for updating to xPlane");
		return -1;
	}

	DLPRINTINFO(2, "STOP");
	return 0;
}

///==================================================================================================
// send a data request to x-plane
// TODO: optimize code
///==================================================================================================
int XpUDP::sendRREF(uint32_t frequency, uint32_t refID, char* xplaneId)
{
	DLVARPRINT(5, "#SEND freq=", frequency);
	DLVARPRINTLN(5, "ID=", refID);
	memset(&_dref_struct_in, 0, sizeof(_dref_struct_in));

	// fill Xplane struct array
	_dref_struct_in.dref_freq = uint2xint(frequency);
	_dref_struct_in.dref_en = uint2xint(refID);
	strcpy(_dref_struct_in.dref_string, xplaneId);

	DLVARPRINT(2, "XPlane Send (udp)    RREF=", _dref_struct_in.dref_en);
	DLVARPRINTLN(2, " FREQ=", _dref_struct_in.dref_freq);
	DLVARPRINT(2, "XPlane Send (native) RREF=", refID);
	DLVARPRINT(2, " FREQ=", frequency);
	DLVARPRINTLN(2, "Element=", xplaneId);

	sendUDPdata(XPMSG_RREF, (byte *)&_dref_struct_in, sizeof(_dref_struct_in), _Xp_dref_in_msg_size);
}
//==================================================================================================
//==================================================================================================
int XpUDP::sendRREF(_DataRefs* newRef)
{
	assert(newRef->paramInfo != NULL);

	if (!newRef->active)
		return -1;

	ESP_LOGD(TAG, "requesting Canid :%d: freq %d Xref:%s:", newRef->paramInfo->xplaneId,
		newRef->frequency, newRef->paramInfo->xplaneId);

	if (newRef->isText)
	{
		char newTxt[400];
		char counter[6];
		int strSize, size;

		ESP_LOGD(TAG, "RREF send txt range:%d to %d", newRef->refID, newRef->endRefID);

		size = newRef->endRefID - newRef->refID;
		DLVARPRINTLN(4, ":size:", size);

		strcpy(newTxt, newRef->paramInfo->xplaneId);
		strSize = strlen(newRef->paramInfo->xplaneId);
		newTxt[strSize] = '[';

		for (int i = 0; i < size; i++)
		{
			esp_task_wdt_reset();
			newTxt[strSize + 1] = '\0';
			itoa(i, counter, 10);
			strcat(newTxt, counter);
			strcat(newTxt, "]");
			ESP_LOGD(TAG, "requesting Xref:%s:", newTxt)
				sendRREF(newRef->frequency, newRef->refID + i, newTxt);
			if ((size % 100) == 0) delay(100);
		}
	}
	else
	{
		DLPRINTLN(4, "RREF send var");
		sendRREF(newRef->frequency, newRef->refID, newRef->paramInfo->xplaneId);
	}
	newRef->subscribed = true;

	newRef->timestamp = millis();
}

//==================================================================================================
//==================================================================================================
int XpUDP::sendDREF(_DataRefs * newRef)
{
	newRef->sendStruct->var = newRef->value;

	DLVARPRINT(4, "#SEND DREF:", newRef->refID); DLPRINTLN(1, newRef->value);

	sendUDPdata(XPMSG_DREF, (byte *)&(newRef->sendStruct), sizeof(_Xp_dref_struct), _Xp_dref_size);
	return 0;
}
//==================================================================================================
//	Send a packet to X-Plane
//==================================================================================================
void XpUDP::sendUDPdata(const char *header, const byte *dataArr, const int arrSize, const int sendSize)
{
	DLPRINTLN(6, "sending XPlane packet...");

	esp_task_wdt_reset();

	// set all bytes in the buffer to 0
	memset(_packetBufferOut, 0, _UDP_MAX_PACKET_SIZE_OUT);
	// Initialize values needed to form request to x-plane

	memcpy(_packetBufferOut, header, 4);
	memcpy((_packetBufferOut)+5, dataArr, arrSize);

	DLVARPRINTLN(6, "Header:", (char *)_packetBufferOut);
	DLPRINTBUFFER(6, _packetBufferOut, sendSize);
	DLVARPRINT(6, "Sending to:", _XPlaneIP);
	DLVARPRINTLN(6, ": Port:", _XpListenPort);
	DLVARPRINTLN(6, "Sending size:", sendSize);

	_Udp.beginPacket(_XPlaneIP, _XpListenPort);
	_Udp.write(_packetBufferOut, sendSize);
	_Udp.endPacket();
}
//==================================================================================================
//
//==================================================================================================