#include "XpUDPdataRef.h"
#include "xpudp_debug.h"

//==================================================================================================
//==================================================================================================

XpUDPdataRef::XpUDPdataRef(int refId)
{
	DLPRINTINFO(2, "START");
	_refID = refId;
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
	_isString = true;
	_txtValue = (char*)malloc(size + 1);
	_newText = (char*)malloc(size + 1);
	memset(_newText, 0, _strSize);
	_txtValue[0] = '\0';
	_newText[0] = '\0';
	_myDataType = XP_DATATYPE_STRING;
	_endRefId = refId + size;
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
	DLPRINTINFO(6, "START");
	_changing = true;
	_newText[index] = (char)newValue;
	DLVARPRINT(6, "idx=", index);
	DLVARPRINTLN(6, "new string=", _newText);
	_receiveCount++;

	if (_receiveCount == _strSize)
	{
		DLVARPRINTLN(6, "String complete:", _newText);
		_changing = false;
		if (!strcmp(_txtValue, _newText))
		{
			strcpy(_txtValue, _newText);
			memset(_newText, 0, _strSize);
			_isChanged = true;
		}
		_receiveCount = 0;  // start again
		if (_callback != NULL)
			_callback((void*)_txtValue);
	}
	DLPRINTINFO(6, "START");
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