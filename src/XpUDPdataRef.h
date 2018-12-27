#ifndef __XPUDPDATAREF_H_
#define __XPUDPDATAREF_H_

#include "XpPlaneInfo.h"
#include <bitset>

//==================================================================================================
class XpUDPdataRef {
public:
	XpUDPdataRef(int refId);
	virtual ~XpUDPdataRef() {};

	virtual int setValue(float newValue, int index = 0) = 0;
	virtual float getValue() const { return _value; };

	bool isChanged() const { return _isChanged; };
	void resetChanged() { _isChanged = false; };

	unsigned long lastTs() { return _timestamp; };
	bool isString()  const { return _isString; };
	uint16_t getCanId() const { return _canId; };
	XPStandardDatatypeID isDataType() { return _myDataType; };
	void setCallback(int(*callback)(void*)) { _callback = callback; };
protected:
	int				_refId = 0;
	uint16_t		_canId;
	uint8_t			_frequency = 5;
	float			_value;
	unsigned long	_timestamp = 0;	// last read time
	bool			_recieveOnly = true;
	bool			_isChanged = false;
	bool			_isString = false;
	XPStandardDatatypeID _myDataType = XP_DATATYPE_FLOAT;
	int(*_callback)(void* newVal) = NULL;
};

//==================================================================================================
class XpUDPstringDataRef : public XpUDPdataRef {
public:

	XpUDPstringDataRef(int refId, int size);
	virtual ~XpUDPstringDataRef();
	int setValue(float newValue, int index);
	virtual char* getCurrentStrValue() const { return _txtValue; };

#define MAXBITSET 512

protected:
	int _endRefId;
	int _strSize;
	int _startValue;
	int _endValue;
	char *_txtValue = NULL;
	char *_newText = NULL;
	std::bitset<MAXBITSET> _gotBit;
};
//==================================================================================================
class XpUDPfloatDataRef : public XpUDPdataRef {
public:

	XpUDPfloatDataRef(int refId) : XpUDPdataRef(refId) {};
	virtual ~XpUDPfloatDataRef() {};
	float getCurrentFloatValue()  const { return _value; };
	virtual int setValue(float newValue, int index = 0) { _value = newValue;	return 0; };
};

#endif
