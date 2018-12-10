#include "XpPlaneInfo.h"
#include "xpudp_debug.h"
#include <stdlib.h>

int XpPlaneInfo::_currentPlane = -1;

XpPlaneInfo::XpPlaneInfo()
{}

//==================================================================================================
//==================================================================================================
XpPlaneInfo::~XpPlaneInfo()
{}
//==================================================================================================
//==================================================================================================
char* XpPlaneInfo::fromCan2Xplane(uint16_t canID)
{
	DPRINTINFO("START");

	if (_currentPlane == -1)
	{
		return 0;
	}

	int i = 0;

	while (suportedPlanes[_currentPlane].canasXplaneTable[i].canasId != 0 && suportedPlanes[_currentPlane].canasXplaneTable[i].canasId != canID)i++;

	DPRINT("returning:"); DPRINTLN(i);

	DPRINTINFO("STOP");

	return suportedPlanes[_currentPlane].canasXplaneTable[i].xplaneId;
}
//==================================================================================================
//==================================================================================================
XplaneTrans* XpPlaneInfo::fromCan2XplaneElement(uint16_t canID)
{
	DPRINTINFO("START");
	XplaneTrans* foundItem;

	if (_currentPlane == -1)
	{
		return NULL;
	}

	int i = 0;

	while (suportedPlanes[_currentPlane].canasXplaneTable[i].canasId != 0 && suportedPlanes[_currentPlane].canasXplaneTable[i].canasId != canID)
	{
		DPRINT("CMP:"); DPRINT(canID); DPRINT("->"); DPRINTLN(suportedPlanes[_currentPlane].canasXplaneTable[i].canasId);
		i++;
	}

	DPRINT("returning:"); DPRINTLN(i);
	foundItem = &(suportedPlanes[_currentPlane].canasXplaneTable[i]);

	DPRINTINFO("STOP");

	return foundItem;
}

XplaneTrans * XpPlaneInfo::getName()
{
	return &getPlaneName;
}

//==================================================================================================
//==================================================================================================
int XpPlaneInfo::findPlane(char * planeName)
{
	DLPRINTINFO(2, "START");
	int res;
	int maxPlanes = sizeof(suportedPlanes) / sizeof(XplanePlanes);

	for (int i = 0; i < maxPlanes; i++)
	{
		res = strncmp(planeName, suportedPlanes[i].name, strlen(suportedPlanes[i].name));
		if (res)
		{
			_currentPlane = i;
			DLVARPRINTLN(2, "found=", i);
			DLVARPRINTLN(2, " score=", res);
			return 0;
		}
	}
	DLPRINTINFO(2, "STOP");
	return -1;
}