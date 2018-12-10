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
#include "XpUDP.h"
#include "xpudp_debug.h"
#include "XpPlaneInfo.h"

//==================================================================================================
// static vars
//--------------------------------------------------------------------------------------------------
//bool	XpUDP::_gotPlaneId = false;
short 	 XpUDP::_RunState = XP_RUNSTATE_STOP;

// inter task stuff
SemaphoreHandle_t xSemaphoreRefs = NULL;

//==================================================================================================
//	constructor
//--------------------------------------------------------------------------------------------------
XpUDP::XpUDP(void(*callbackFunc)(uint16_t canId, float par), void(*setStateFunc)(bool state))
{
	DLPRINTINFO(2, "START");

	_callbackFunc = callbackFunc;
	_setStateFunc = setStateFunc;
	DLPRINTLN(1, "set state to XP_RUNSTATE_STOP");
	_RunState = XP_RUNSTATE_STOP;

	vSemaphoreCreateBinary(xSemaphoreRefs);

	DLPRINTINFO(2, "STOP");
}

//==================================================================================================
//	start the session with the X-Plane server
// TODO: check where used
//==================================================================================================
int XpUDP::start()
{
	int 	result;

	// check if bus is already running
	DLPRINTINFO(2, "START");

	if (_RunState >= XP_RUNSTATE_GOT_BEACON)
	{
		DLPRINTINFO(0, "!!STOPPING: Is running");
		return -3;
	}

	if (_startUDP())
	{
		GetBeacon();
	}

	DLPRINTINFO(2, "STOP");
	return result;
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
//	data reader for in loop() pols the udp port for new messages and dispaces them to controls
// TODO: process arrays
// TODO: create task
//==================================================================================================
int XpUDP::dataReader()
{
	int message_size = 0;
	int noBytes = 0;
	_DataRefs* tmpRef;

	long startTs = millis();
	DLPRINTINFO(2, "START");

	switch (_RunState)
	{
	case XP_RUNSTATE_UDP_UP:
		if (_Xp_beacon_restart_timeoutMs < (startTs - _last_start))
			GetBeacon();
		return -1;
		break;
	case XP_RUNSTATE_IS_CLEANING:
		if (_Xp_initial_clean_timeoutMs < (startTs - _startCleaning))
		{
			DLPRINTLN(2, "End cleaning");
			DLPRINTLN(1, "set state to XP_RUNSTATE_DONE_CLEANING");
			_RunState = XP_RUNSTATE_DONE_CLEANING;
			// ask for plane id
			registerDataRef(XpPlaneInfo::getName(), true, _processPlaneIdent);// todo get value
		}
		break;
	case XP_RUNSTATE_GOT_PLANE:
		unRegisterDataRef(XpPlaneInfo::getName()->canasId);
		_setStateFunc(true);  // signal to canbus interface that xplane interface is running
		DLPRINTLN(1, "Set state to XP_RUNSTATE_RUNNING");
		_RunState = XP_RUNSTATE_RUNNIG;
		break;
	case XP_RUNSTATE_RUNNIG:
		if (_Xp_beacon_timeoutMs < (startTs - _lastXpConnect))
		{
			DLPRINTLN(1, "Set state to XP_RUNSTATE_UDP_UP");
			_RunState = XP_RUNSTATE_UDP_UP;
		}
		break;

	default:
		break;
	}

	_LastError = XpUDP::XP_ERR_SUCCESS;
	DLPRINT(5, "start parse");
	noBytes = _Udp.parsePacket(); //// returns the size of the packet
	DLPRINT(5, ":end parse");

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

	// for now
	// TODO: move to seperate tasks
	checkDataRefQueue();
	checkReceiveData();

	DLPRINTINFO(2, "STOP");
	return _LastError;
}
//==================================================================================================
// process new plane identification string
//==================================================================================================
int XpUDP::_processPlaneIdent(void* newInfo)
{
	DLPRINTINFO(2, "START");

	char* newPlane = (char*)newInfo;

	DLVARPRINTLN(2, "Testing:", newPlane);

	if (XpPlaneInfo::findPlane(newPlane) == 0)
	{
		DLPRINTLN(1, "Set state to XP_RUNSTATE_GOT_PLANE");
		_RunState = XP_RUNSTATE_GOT_PLANE;
		DLPRINTLN(2, "Plane Found");
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
	DLPRINTINFO(2, "START");
	IPAddress   XpMulticastAdress(XP_MulticastAdress_0, XP_MulticastAdress_1, XP_MulticastAdress_2, XP_MulticastAdress_3);
	uint16_t  	XpMulticastPort = XP_Multicast_Port;

	if (_RunState > XP_RUNSTATE_STOP)
	{
		return -1;
	}
	_last_start = millis();

	_LastError = XpUDP::XP_ERR_SUCCESS;

	// start connection
	DLVARPRINT(1, "Starting connection, listening on:", XpMulticastAdress); DLVARPRINTLN(1, " Port:", XpMulticastPort);

	if (_Udp.beginMulticast(XpMulticastAdress, XpMulticastPort))
	{
		DLPRINTLN(1, "Set state to XP_RUNSTATE_UDP_UP");
		_RunState = XP_RUNSTATE_UDP_UP;

		DLPRINTLN(1, "Udp start ok");
	}
	else
	{
		DLPRINTLN(0, "!!Udp start failed");
		return  -1;
	}

	DLPRINTINFO(2, "STOP");
	return(0);
};
//==================================================================================================
//	get beacon from X Plane server and store adres and port
//==================================================================================================
int XpUDP::GetBeacon(long howLong)
{
	DLPRINTINFO(2, "START");
	long startTs = millis();
	int result;

	if (_RunState < XP_RUNSTATE_UDP_UP)
	{
		DLVARPRINTLN(0, "Bad Runstate:", _RunState);
		return -1;
	}

	_LastError = XpUDP::XP_ERR_SUCCESS;

	// wait for beacon transmision
	do
	{
		DLPRINT(4, "*");
		result = _ReadBeacon();
		DLVARPRINTLN(4, "Result:", result);
		esp_task_wdt_reset();
		delay(10);
	} while (result != 0 && ((millis() - startTs) < howLong));

	return 0;
	DLPRINTINFO(2, "Stop");
}
//------------------------------------------------------------------------------------------------------------
int XpUDP::_ReadBeacon()
{
	int message_size;
	int noBytes;
	long startTs = millis();
	int result = XpUDP::XP_ERR_SUCCESS;

	_LastError = XpUDP::XP_ERR_SUCCESS;

	DLPRINTINFO(2, "START");

	DLPRINTLN(3, "Search for beacon");

	noBytes = _Udp.parsePacket(); //// returns the size of the packet

	if (noBytes > 0)
	{
		DLPRINT(2, millis() / 1000);
		DLVARPRINT(2, ":Packet of ", noBytes);
		DLVARPRINT(2, " received from ", _Udp.remoteIP());
		DLVARPRINTLN(2, ":", _Udp.remotePort());

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
#if DEBUG_LEVEL > 0
					Serial.println("UDP beacon received");
					Serial.print("beacon_major_version:");	Serial.println(_becn_struct.beacon_major_version);
					Serial.print("beacon_minor_version:");	Serial.println(_becn_struct.beacon_minor_version);
					Serial.print("application_host_id:");	Serial.println(_becn_struct.application_host_id);
					Serial.print("version_number:");		Serial.println(_becn_struct.version_number);
					Serial.print("application_host_id:");	Serial.println(_becn_struct.application_host_id);
					Serial.print("version_number:");		Serial.println(_becn_struct.version_number);
					Serial.print("role:");					Serial.println(_becn_struct.role);
					Serial.print("port:");					Serial.println(_becn_struct.port);
					Serial.print("computer_name:");			Serial.println(_becn_struct.computer_name);
#endif
					if (_becn_struct.beacon_major_version != 1 || _becn_struct.beacon_minor_version != 2)
					{
						DLPRINTLN(0, "!!Beacon: Version wrong");
						DLVARPRINT(0, "Version found: ", _becn_struct.beacon_major_version);
						DLPRINTLN(0, _becn_struct.beacon_minor_version);

						_LastError = XpUDP::XP_ERR_WRONG_HEADER;
					}
					// store listen port
					_XpListenPort = _becn_struct.port;

					// correct start so update status
					_lastXpConnect = millis();
					DLPRINTLN(1, "Set state to XP_RUNSTATE_IS_CLEANING");
					_RunState = XP_RUNSTATE_IS_CLEANING;
					_startCleaning = _lastXpConnect;

					// inform CANBUS Manager that system is running
					if (_setStateFunc != NULL)
						_setStateFunc(_RunState);
				}
				else
				{
					DLVARPRINTLN(0, "!!Beacon: Header wrong", _becn_struct.header);

					_LastError = XpUDP::XP_ERR_WRONG_HEADER;
				}
			}
			else
			{
				DLPRINTLN(0, "!!Beacon: Message size to big for struct");

				_LastError = XpUDP::XP_ERR_BUFFER_OVERFLOW;
			}
		}
		else
		{
			DLPRINTLN(0, "!!Beacon: Message size to big for buffer");

			_LastError = XpUDP::XP_ERR_BUFFER_OVERFLOW;
		}
	}
	else
	{
		DLPRINTLN(4, "Beacon: Nothing to read");
		DLPRINT(4, ">>Beacon not found");
		_LastError = XpUDP::XP_ERR_BEACON_NOT_FOUND;
	}
	// everything ok :-)

	DLPRINTINFO(2, "STOP");
	// debug
	//_XpListenPort = 49000;
	//_XPlaneIP = IPAddress(192, 168, 9, 121);
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
};
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

		DLVARPRINT(4, "XPlane Received (native) RREF=", en);
		DLVARPRINTLN(4, " value=", value);

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
				DLVARPRINT(4, "Code found in subscription list i=", i);
				DLVARPRINTLN(4, " canID=", curRef->canId);
				curRef->timestamp = millis();

				assert(curRef->dataRef != NULL);

				if (!curRef->isText)
				{
					DLPRINT(5, "not text");

					DLVARPRINTLN(5, " Value=", curRef->dataRef->getValue());

					if (true) //(curRef->value != value)
					{
						DLVARPRINTLN(5, "XPlane Updating value to:", value);

						// TODO: Call to update function
						// canid's < 200 are usd for info only relevant to XpUDP interface like plane name
						if ((curRef->paramInfo->canasId > 200) && (_callbackFunc != NULL))
						{
							_callbackFunc(curRef->paramInfo->canasId, value);
						}
						else
							DLPRINTLN(0, "!! NO Callback");

						//if (curRef->setdata) {
						//	curRef->setdata(value);
					}
					curRef->dataRef->setValue(value);
				}
				else
				{
					DLPRINT(4, "is text");

					((XpUDPstringDataRef*)curRef->dataRef)->setValue(value, (en - curRef->refID));
					if (curRef->dataRef->isChanged())
					{
						// TODO:if changed
					}

					DLVARPRINTLN(4, "new Str Value=", ((XpUDPstringDataRef*)curRef->dataRef)->getCurrentStrValue());
				}
			}
			else
			{
				// not found in subscription list so sign out of a RREF
				DLVARPRINTLN(4, "!!Code not found in subscription list or cleaning up, signing out of RREF:", en);
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
				}

				if (tmpRef->active && (tmpRef->paramInfo != NULL) && ((timeNow - tmpRef->timestamp) > _MAX_INTERVAL))
				{
					DLVARPRINTLN(0, "!!Item not receiving data (resend RREF):", i);
					DLVARPRINT(1, "interval=", _MAX_INTERVAL); DLVARPRINT(1, " now=", timeNow); DLVARPRINTLN(1, " ts=", tmpRef->timestamp);

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
	DLPRINTINFO(2, "START");

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
			DLPRINTLN(1, "adding dataref");
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
		DLPRINTINFO(2, "STOP no record");
		return -1;
	}

	DLPRINTINFO(2, "STOP");
	return 0;
}
//==================================================================================================
int XpUDP::_registerNewDataRef(uint16_t canasId, bool recieveOnly, int(*callback)(void* value), XplaneTrans *newInfo)
{
	int curRecord = -1;
	_DataRefs* newRef = NULL;

	DLPRINTINFO(2, "START");

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
		DLPRINTLN(1, "New record");
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

	DLVARPRINTLN(2, "infoRecord=", (newInfo == NULL) ? 0 : 1);

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
		DLPRINTLN(1, "not subscribing");
	}

	xSemaphoreGive(xSemaphoreRefs);
	DLPRINTINFO(2, "STOP");

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
	DLPRINTINFO(2, "START by canId");

	DLVARPRINTLN(1, "Registering:", canasId);
	// check if struct is free to manipulate

	assert(_registerNewDataRef(canasId, recieveOnly, callback, NULL) == XP_ERR_SUCCESS);

	DLPRINTINFO(2, "STOP");

	return 0;
}

//==================================================================================================
//
// the CanID will be the unoique ref for all datarequests in the system
// in this struct we will only register datarefs we need from XPlane. // ??
/// curXpInfo  : pointer to description struct
/// receiveOnly : if true we dont need to put this in the CanBus queue but will use an internal callback
/// callback : internal function to directly process the value (like for the plane name).
//==================================================================================================
/*
int XpUDP::registerDataRef(XplaneTrans* curXPinfo, bool recieveOnly, int(*callback)(void* value))
{
	bool notFound = true;
	int curRecord = -1;
	_DataRefs* newRef;
	_DataRefs* tmpRef;

	DLPRINTINFO(2, "START");
	// first test if we already have the item.

	// check if active and for last timestamp
	if ((newRef->recieveOnly) && (!newRef->subscribed || (millis() - newRef->timestamp) > _MAX_INTERVAL))
	{
		// create new request
		DLPRINTLN(1, "create x-plane request");
		newRef->subscribed = true;
		sendRREF(newRef);

		// read first returned value
		dataReader();
	}
	else
	{
		DLPRINTLN(1, "not subscribing");
	}

	if (!newRef->recieveOnly)
	{
		strcpy(newRef->sendStruct.dref_path, curXPinfo->xplaneId);
	}
	DLPRINTINFO(2, "STOP");

	return (int)XpUDP::XP_ERR_SUCCESS;
}
*/
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

			DLVARPRINTLN(1, "Unregistering:", tmpRef->canId);
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
	DLVARPRINT(4, "#SEND RREF:", newRef->refID);

	assert(newRef->paramInfo != NULL);

	DLVARPRINTLN(4, ":", newRef->paramInfo->xplaneId);

	if (!newRef->active)
		return -1;

	if (newRef->isText)
	{
		char newTxt[400];
		char counter[6];
		int strSize, size;

		DLVARPRINT(4, "RREF send txt range:", newRef->refID);
		DLVARPRINT(4, ":to:", newRef->endRefID);
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
			sendRREF(newRef->frequency, newRef->refID + i, newTxt);
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
/*
int XpUDP::sendRREF(_DataRefs * newRef, int size)
{
	DLPRINTINFO(2, "START");
	DLVARPRINT(1, "New RREF request:", newRef->refID); DLVARPRINTLN(1, ":", newRef->paramInfo->xplaneId);

	char counter[32];
	int startPos = strlen(newRef->paramInfo->xplaneId);
	DLVARPRINTLN(1, "size=:", startPos);

	memset(&_dref_struct_in, 0, sizeof(_dref_struct_in));
	_dref_struct_in.dref_freq = uint2xint(newRef->frequency);
	strcpy(_dref_struct_in.dref_string, newRef->paramInfo->xplaneId);

	DLVARPRINT(1, "#SEND DREF array:", newRef->refID); DLPRINTLN(1, newRef->value);
	DLVARPRINTLN(1, "#SEND freq=", newRef->frequency);

	_dref_struct_in.dref_string[startPos] = '[';

	for (int i = 0; i < size; i++)
	{
		_dref_struct_in.dref_string[startPos + 1] = '\0';
		itoa(i, counter, 10);
		strcat(_dref_struct_in.dref_string, counter);
		strcat(_dref_struct_in.dref_string, "]");

		_dref_struct_in.dref_en = uint2xint(newRef->refID + i);
		DLVARPRINT(1, "ID=", newRef->refID + i);
		DLVARPRINTLN(1, ":", _dref_struct_in.dref_string);

		sendUDPdata(XPMSG_RREF, (byte *)&_dref_struct_in, sizeof(_dref_struct_in), _Xp_dref_in_msg_size);
	}

	newRef->timestamp = millis();
	DLPRINTINFO(2, "STOP");
	return 0;
}
*/
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