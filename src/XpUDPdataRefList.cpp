#define LOG_LOCAL_LEVEL 5
static const char *TAG = "XpUDPDataRefList";

#include "XpUDPdataRefList.h"
#include "XpUDPdataRef.h"
#include "esp_log.h"
#include "xpudp_debug.h"

//==================================================================================================
// deconstructor clean everything up
//--------------------------------------------------------------------------------------------------
XpUDPdataRefList::~XpUDPdataRefList()
{
	// TODO: cleanup all storage
	for (int i = 0; i < _listRefs.length(); i++)
	{
		if (_listRefs[i]->dataRef != NULL) delete _listRefs[i]->dataRef;
		if (_listRefs[i]->sendStruct != NULL) free(_listRefs[i]->sendStruct);
	}
	_listRefs.clear();
}
//==================================================================================================
// add new item to list
//--------------------------------------------------------------------------------------------------
XpDataRef* XpUDPdataRefList::addNew(uint16_t canID, bool isRREF, int(*callback)(void* value), XplaneTrans *newInfo)
{
	XpDataRef *myRef = new XpDataRef;

	myRef->canId = canID;
	myRef->callback = callback;
	myRef->isRREF = isRREF;

	ESP_LOGV(TAG, "adNew newinfo ? %s", (newInfo == NULL) ? "NULL" : "VALUE");

	if (!myRef->isRREF)
	{
		myRef->sendStruct = (_Xp_dref_struct*)calloc(1, sizeof(_Xp_dref_struct));
	}

	_getTrans(myRef, false, newInfo);

	_listRefs.push_back(myRef);

	return myRef;
}

//==================================================================================================
// TODO: adapt
//--------------------------------------------------------------------------------------------------
void XpUDPdataRefList::checkAll(bool newPlane)
{
	for (int i = 1; i < _listRefs.length(); i++)
	{
		if (_listRefs[i]->isActive && (newPlane || (_listRefs[i]->paramInfo == NULL)))
		{
			_getTrans(_listRefs[i], newPlane);
		}
	}
}
//==================================================================================================
// check if trans record available and add datarefs
//--------------------------------------------------------------------------------------------------
void XpUDPdataRefList::_getTrans(XpDataRef* myRef, bool newPlane, XplaneTrans *newInfo)
{
	ESP_LOGV(TAG, "getting trans for %d with %s", myRef->canId, ((newInfo == NULL) ? "NULL" : "TRANS"));

	if (newInfo != NULL)
	{
		myRef->paramInfo = newInfo;
	}
	else
	{
		myRef->paramInfo = XpPlaneInfo::fromCan2XplaneElement(myRef->canId);
	}

	if (myRef->paramInfo != NULL)
	{
		ESP_LOGV(TAG, "Adding param");

		myRef->refID = _lastRefId++;
		if (!myRef->isRREF)
		{
			ESP_LOGV(TAG, "Adding ading receive info");
			strcpy(myRef->sendStruct->dref_path, myRef->paramInfo->xplaneId);
		};

		if (myRef->dataRef == NULL)
		{
			ESP_LOGV(TAG, "Adding dref");
			if (myRef->paramInfo->xpDataType == XP_DATATYPE_STRING)
			{
				XpUDPstringDataRef* newRef = new XpUDPstringDataRef(myRef->refID, myRef->paramInfo->stringSize);
				myRef->endRefID = myRef->refID + myRef->paramInfo->stringSize;
				_lastRefId = myRef->endRefID + 1;
				myRef->dataRef = (XpUDPdataRef*)newRef;
			}
			else
			{
				XpUDPfloatDataRef* newRef = new XpUDPfloatDataRef(myRef->refID);
				myRef->dataRef = (XpUDPdataRef*)newRef;
			}

			myRef->dataRef->setCallback(myRef->callback);
		}
		else
		{
			// TODO: check what is changed....
			assert(myRef->dataRef->isDataType() == myRef->paramInfo->xpDataType);
		}
	}
	else
	{
		if (newPlane)
			myRef->isActive = false;
	}
}

//==================================================================================================
// TODO: adapt
//--------------------------------------------------------------------------------------------------
XpDataRef* XpUDPdataRefList::findById(int refId, bool checkAll)
{
	XpDataRef *tmpRef;

	for (int i = 0; i < _listRefs.size(); i++)
	{
		tmpRef = _listRefs[i];
		if ((tmpRef->isActive || checkAll) && (tmpRef->refID <= refId) && (tmpRef->endRefID >= refId))
		{
			return tmpRef;
			break;
		}
	}
	return NULL;
}

//==================================================================================================
// TODO: adapt
//--------------------------------------------------------------------------------------------------
XpDataRef* XpUDPdataRefList::findByCanId(uint16_t CanId, bool checkAll)
{
	XpDataRef *tmpRef;

	for (int i = 0; i < _listRefs.size(); i++)
	{
		tmpRef = _listRefs[i];
		if ((tmpRef->isActive || checkAll) && tmpRef->canId == CanId)
		{
			return tmpRef;
			break;
		}
	}
	return NULL;
}
//==================================================================================================
//==================================================================================================
XpDataRef* XpUDPdataRefList::unRegister(uint16_t canID)
{
	XpDataRef *tmpRef = findByCanId(canID);

	if (tmpRef != NULL)
	{
		tmpRef->isActive = false;
	}

	return tmpRef;
}

//==================================================================================================
//==================================================================================================
int XpUDPdataRefList::_getDataRefInfo(XpDataRef* newRef, XplaneTrans* thisXPinfo)
{
	if (newRef->paramInfo == NULL)
	{
		XplaneTrans* curXPinfo = NULL;
		// find record
		if (thisXPinfo == NULL)
		{
			curXPinfo = XpPlaneInfo::fromCan2XplaneElement(newRef->canId);
		}
		else
			curXPinfo = thisXPinfo;

		newRef->paramInfo = curXPinfo;
	}

	if (newRef->paramInfo != NULL)
	{
		if (newRef->paramInfo->xpDataType == XP_DATATYPE_STRING)
		{
			newRef->endRefID += newRef->paramInfo->stringSize;
			_lastRefId = newRef->endRefID + 1;
		}

		if (newRef->dataRef == NULL)
		{
			ESP_LOGD(TAG, "Adding new dataref for %d", newRef->canId);

			if (newRef->paramInfo->xpDataType == XP_DATATYPE_STRING)
			{
				newRef->dataRef = new XpUDPstringDataRef(newRef->refID, newRef->paramInfo->stringSize);
			}
			else
			{
				newRef->dataRef = new XpUDPfloatDataRef(newRef->refID);
			}
			if (newRef->callback != NULL)
				newRef->dataRef->setCallback(newRef->callback);
		}

		if (!newRef->isRREF)
		{
			newRef->sendStruct = (_Xp_dref_struct*)calloc(1, sizeof(_Xp_dref_struct));
			strcpy(newRef->sendStruct->dref_path, newRef->paramInfo->xplaneId);
		}
	}
	else
	{
		ESP_LOGE(TAG, "STOP no dataref info found for %d", newRef->canId);
		return -1;
	}

	return 0;
}
//==================================================================================================
	// now loop to check if we are receiving all data
	// TODO: make task
//--------------------------------------------------------------------------------------------------
int XpUDPdataRefList::checkReceiveData(long timeNow, int(*sendRREF)(XpDataRef* newRef))
{
	XpDataRef* tmpRef;

	ESP_LOGV(TAG, "########## check receive loop ########## =%d", _listRefs.size());

	for (int i = 0; i < _listRefs.size(); i++)
	{
		tmpRef = _listRefs[i];

		// check if we have a info record
		if (tmpRef->paramInfo == NULL)
		{
			_getTrans(tmpRef);
		}

		ESP_LOGV(TAG, "%d: %s active paramInfo=%s last=%d", tmpRef->canId, (tmpRef->isActive) ? "Is" : "not",
			(tmpRef->paramInfo != NULL) ? "OK" : "NULL", tmpRef->timestamp);

		if (tmpRef->isActive && (tmpRef->paramInfo != NULL) &&
			((timeNow - tmpRef->timestamp) > _MAX_INTERVAL))
		{
			ESP_LOGD(TAG, "!!Item not receiving data (resend RREF):%d interval=%d now=%d ts=%d",
				i, _MAX_INTERVAL, timeNow, tmpRef->timestamp);

			sendRREF(tmpRef);
			tmpRef->timestamp = timeNow;
		}
	}
}