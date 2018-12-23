#ifndef __XPUDPDATAREF_H_
#define __XPUDPDATAREF_H_

#include "XpPlaneInfo.h"
#include <bitset>
//==================================================================================================
class XpUDPdataRef {
public:

	XpUDPdataRef(int refId);
	virtual ~XpUDPdataRef();

	virtual int setValue(float newValue, int index = 0) = 0;
	virtual float getValue() const { return _value; };
	bool isActive()  const { return _isActive; };
	bool isChanged() const { return _isChanged; };
	unsigned long lastTs() { return _timestamp; };
	void setCallback(int(*callback)(void*)) { _callback = callback; };
	bool isString()  const { return _isString; };
	int getId() const { return _refID; };
	XPStandardDatatypeID isDataType() { return _myDataType; };

protected:
	int				_refID;	//Internal number of dataref
	uint8_t			_frequency = 5;
	float			_value;
	bool			_isActive = true;
	unsigned long	_timestamp = 0;	// last read time
	uint16_t		_canId = 0;
	XplaneTrans*	_paramInfo = NULL;
	bool			_recieveOnly = true;
	bool			 _isChanged = false;
	bool			_isString = false;
	int(*_callback)	(void* newVal);
	XPStandardDatatypeID _myDataType = XP_DATATYPE_FLOAT;
};
//==================================================================================================
class XpUDPstringDataRef : public XpUDPdataRef {
public:

	XpUDPstringDataRef(int refId, int size);
	virtual ~XpUDPstringDataRef();
	int setValue(float newValue, int index);
	virtual char* getCurrentStrValue();

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

	XpUDPfloatDataRef(int refId);
	virtual ~XpUDPfloatDataRef();
	float getCurrentFloatValue()  const { return _value; };
	virtual int setValue(float newValue, int index = 0);

protected:
	int(*_callback)(float newVal);
};

#endif
