#define LOG_LOCAL_LEVEL 5
static const char *TAG = "XpUDPdataRef";

#include "XpUDPdataRef.h"
#include "xpudp_debug.h"
#include "esp_log.h"

//==================================================================================================
//==================================================================================================

XpUDPdataRef::XpUDPdataRef(int refId)
{
	DLPRINTINFO(2, "START");
	_refID = refId;
	_timestamp = millis();
	DLPRINTINFO(2, "STOP");
}

//==================================================================================================
//==================================================================================================

XpUDPdataRef::~XpUDPdataRef()
{}
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
	_txtValue = (char*)malloc(size + 1);
	_newText = (char*)malloc(size + 1);
	memset(_newText, 0, _strSize);
	_txtValue[0] = '\0';
	_newText[0] = '\0';
	_myDataType = XP_DATATYPE_STRING;
	_endRefId = refId + size;
	// set bit array for all posible

	_gotBit.reset();
	for (int i = 0; i < _strSize; i++) _gotBit.set(i);

	DLPRINTINFO(2, "STOP");
}

//==================================================================================================
//==================================================================================================
XpUDPstringDataRef::~XpUDPstringDataRef()
{
	DLPRINTINFO(2, "START");
	if (_txtValue != NULL)	free(_txtValue);
	if (_newText != NULL)	free(_newText);
	DLPRINTINFO(2, "STOP");
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
	if (_gotBit.count() < 5)
	{
		Serial.println((char*)_gotBit.to_string().c_str());
	}
	//ESP_LOGD(TAG, "Current bitmask:%s:", _gotBit.to_string());

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
			_callback((void*)_txtValue);
		}
	}

	return 0;
}

//==================================================================================================
//==================================================================================================
char * XpUDPstringDataRef::getCurrentStrValue()
{
	return _txtValue;
}

//==================================================================================================
//==================================================================================================
//==================================================================================================
//==================================================================================================
XpUDPfloatDataRef::XpUDPfloatDataRef(int refId) : XpUDPdataRef(refId)
{}

XpUDPfloatDataRef::~XpUDPfloatDataRef()
{}

int XpUDPfloatDataRef::setValue(float newValue, int index)
{
	_value = newValue;
	return 0;
}