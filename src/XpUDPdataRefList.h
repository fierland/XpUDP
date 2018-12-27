#ifndef __XPUDPDATAREFLIST_H_
#define __XPUDPDATAREFLIST_H_

#include "XPUtils.h"
#include "XpUDPdataRef.h"
#include "XpPlaneInfo.h"
#include "QList.h"

struct XpDataRef {
	int				refID = 0;	//Internal number of dataref
	int				endRefID = 0;	//Internal last number of dataref
	uint16_t		canId = 0;
	bool			isActive = true;  // is active and not unsubscribed
	unsigned long	timestamp = 0;		// last read time
	XplaneTrans*	paramInfo = NULL;
	XpUDPdataRef*   dataRef = NULL;
	bool			isRREF = true;  // RREF record and not DREF
	int(*callback)(void* newVal) = NULL;  // posible call back to update value
	bool			 isChanged = false;
	//XPStandardDatatypeID _myDataType = XP_DATATYPE_FLOAT;
	bool			subscribed = false;  // RREF send to XPlane
	_Xp_dref_struct*  sendStruct = NULL;
};

class XpUDPdataRefList {
public:

	XpUDPdataRefList() {};
	~XpUDPdataRefList();

	XpDataRef* findById(int refId, bool checkAll = false);
	XpDataRef* findByCanId(uint16_t canId, bool checkAll = false);

	XpDataRef* unRegister(uint16_t canID);

	XpDataRef* addNew(uint16_t canID, bool isRREF = true, int(*callback)(void* value) = NULL, XplaneTrans *newInfo = NULL);

	void checkAll(bool newPlane = false);
	int checkReceiveData(long timeNow, int(*sendRREF)(XpDataRef *newRef));

protected:
	static const int	_MAX_INTERVAL = 30 * 1000; 		// Max interval in ms between data messages
	void _getTrans(XpDataRef * myRef, bool newPlane = false, XplaneTrans *newInfo = NULL);
	int _getDataRefInfo(XpDataRef * newRef, XplaneTrans * thisXPinfo);

	int _unRegisterDataRefItem(uint16_t canId);
	//
	// a place to hold all referenced items
	//
	QList<XpDataRef *> _listRefs;
	uint32_t _lastRefId = 1;

private:
};

#endif
