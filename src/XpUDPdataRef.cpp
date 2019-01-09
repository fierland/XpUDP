#define LOG_LOCAL_LEVEL 5
static const char *TAG = "XpUDPdataRef";

#include <stdlib.h>
#include <string.h>
#include "esp32-hal.h"
#include "XpUDPdataRef.h"
#include "xpudp_debug.h"
#include "esp_log.h"

//==================================================================================================
//==================================================================================================

XpUDPdataRef::XpUDPdataRef(int refId)
{
	DLPRINTINFO(2, "START");
	_refId = refId;
	_timestamp = millis();
	DLPRINTINFO(2, "STOP");
}

//==================================================================================================
//==================================================================================================

int XpUDPdataRef::setValue(float newValue, int index)
{
	_timestamp = millis();
	_value = newValue;

	return 0;
}

//==================================================================================================
// dataref class for string values
//==================================================================================================
//==================================================================================================
XpUDPstringDataRef::XpUDPstringDataRef(int refId, int size) : XpUDPdataRef(refId)
{
	DLPRINTINFO(2, "START");
	_strSize = size;
	assert(size <= MAXBITSET);

	_isString = true;
	_txtValue = (char*)calloc(1, size + 1);
	_newText = (char*)calloc(1, size + 1);
	memset(_newText, 0, _strSize);
	_txtValue[0] = '\0';
	_newText[0] = '\0';
	_myDataType = XP_DATATYPE_STRING;
	_endRefId = refId + size;
	// set bit array for all posible

	_gotBit.reset();
	for (int i = 0; i < _strSize; i++) _gotBit.set(i);

	ESP_LOGD(TAG, "created new string statref for %d size=%d", refId, _strSize);

	DLPRINTINFO(2, "STOP");
}

//==================================================================================================
//==================================================================================================
XpUDPstringDataRef::~XpUDPstringDataRef()
{
	if (_txtValue != NULL)	free(_txtValue);
	if (_newText != NULL)	free(_newText);
}

//==================================================================================================
//==================================================================================================
int XpUDPstringDataRef::setValue(float newValue, int index)
{
	_newText[index] = (char)newValue;
	_gotBit.reset(index);

	ESP_LOGV(TAG, "replacing idx=%d with [%c] :%d chars left ", index, (char)newValue, _gotBit.count());
	ESP_LOGV(TAG, "String =:%s:", _newText);

	// all items received for a string
//#if (DEBUG_LEVEL > LOG_DEBUG)
//	if (_gotBit.count() < 20)
//	{
//		Serial.println((char*)_gotBit.to_string().c_str());
//	}
//	//ESP_LOGD(TAG, "Current bitmask:%s:", _gotBit.to_string());
//#endif

	if (!_gotBit.any())
	{
		for (int i = 0; i < _strSize; i++) _gotBit.set(i);
		_timestamp = millis();

		ESP_LOGV(TAG, "String complete:%s:", _newText);

		if (strlen(_txtValue) == 0 || !strcmp(_newText, _txtValue))
		{
			ESP_LOGV(TAG, "strcpy");
			strcpy(_txtValue, _newText);
			//memset(_newText, 0, _strSize);
			_isChanged = true;
		}
		ESP_LOGV(TAG, "New string :%s:", _txtValue);

		if (_callback != NULL)
		{
			if (!_callback((void*)_txtValue) != 0) _isChanged = false;
		}
	}

	return 0;
}

//==================================================================================================
//==================================================================================================