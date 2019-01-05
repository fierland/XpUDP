//=================================================================================================
//
//
//=================================================================================================
// planeinfo.ini file contents
//
//# see CanasId.h for codes
//[Cessna 172 SP Skyhawk - 180HP]
//item = 201, "laminar/c172/fuel/fuel_quantity_L", "FLOAT", 2, 1
//item = 202, "laminar/c172/fuel/fuel_quantity_R", "FLOAT", 2, 1
//item = 203, "sim/cockpit2/indicators/EGT_deg_C[0]", "FLOAT", 2, 1
//item = 204, "sim/cockpit2/engine/indicators/fuel_flow_kg_sec[0]", "FLOAT", 2, 1
//item = 205, "laminar/c172/electrical/battery_amps", "FLOAT", 2, 1
//item = 208, "sim/cockpit/misc/vacuum", "FLOAT", 2, 1
//item = 206, "sim/cockpit2/indicators/oil_temperature_deg_C[0]", "FLOAT", 2, 1
//item = 207, "sim/cockpit2/indicators/oil_pressure_psi[0]", "FLOAT", 2, 1
//item = 1800, "sim/cockpit/electrical/instrument_brightness", "FLOAT", 2, 1
//item = 209, "172/ip_base/b_avionics_master", "FLOAT", 5, 1
//
//[Airfoillabs Cessna 172SP]
//item = 201, "172/instruments/uni_fuel_L_x", "FLOAT", 2, 1
//item = 202, "172/instruments/uni_fuel_R_x", "FLOAT", 2, 1
//item = 203, "172/instruments/uni_EGT", "FLOAT", 2, 1
//item = 204, "172/instruments/uni_FF", "FLOAT", 2, 1
//item = 205, "172/instruments/uni_bat_amps", "FLOAT", 2, 1
//item = 208, "172/instruments/uni_suction", "FLOAT", 2, 1
//item = 206, "172/instruments/uni_oil_F", "FLOAT", 2, 1
//item = 207, "172/instruments/uni_oil_pres", "FLOAT", 2, 1
//item = 1800, "172/lights/int/int_radio_panel_lt", "FLOAT", 2, 1
//item = 209, "sim/cockpit/electrical/battery_on", "FLOAT", 5, 1

#ifndef __XP_PLANEINFO_H_
#define __XP_PLANEINFO_H_

#include <CanasId.h>
#include <stdlib.h>
#include <stdint.h>
#include "QList.h"

typedef enum {
	XP_DATATYPE_FLOAT,
	XP_DATATYPE_STRING,
	XP_DATATYPE_BOOL,
	XP_DATATYPE_BITS,
	XP_DATATYPE_INT
} XPStandardDatatypeID;

struct XplaneTrans {
	uint16_t	canasId = 0;
	char*		xplaneId = NULL;
	int			xpDataType = XP_DATATYPE_STRING;
	int			xpTimesSecond = 2;
	float		conversionFactor = 1;
	int			stringSize = 260;
};

struct XplanePlanes {
	char* name;
	XplaneTrans canasXplaneTable[MAX_XP_CAN_ITEMS];
};

//static XplaneTrans getPlaneName = { CANAS_NOD_PLANE_NAME,"sim/aircraft/view/acf_descrip",XP_DATATYPE_STRING,2,1,260 };

//==================================================================================================
//==================================================================================================
class XpPlaneInfo {
public:

	XpPlaneInfo() {};
	~XpPlaneInfo() {};

	//	static  XplaneTrans* getName() { return &getPlaneName; };

	static char* fromCan2Xplane(uint16_t canID);
	static  XplaneTrans* fromCan2XplaneElement(uint16_t canID);
	static int findPlane(char* planeName);
	static int setIniFile(const char* fileName);

private:
	//static int _currentPlane;
	static bool _PlaneLoaded;
	static char* _iniFile;

	struct _planeInfo {
		char* name = NULL;
		unsigned long filePos;
	};

	//static QList<_planeInfo *> _planeInfoItems;
	static QList<XplaneTrans*> _planeTranslations;

	static int _readIniFile(char * planeName);
	static char* _getNext(char * buffer, char terminator, char * newVal);
	static char * trimQuotes(char * inStr);
	static int _findPlaneItem(uint16_t canID);
};
#endif
