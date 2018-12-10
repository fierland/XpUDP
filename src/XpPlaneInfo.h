#ifndef __XP_PLANEINFO_H_
#define __XP_PLANEINFO_H_

#include <CanasId.h>
#include <stdlib.h>
#include <stdint.h>

typedef enum {
	XP_DATATYPE_FLOAT,
	XP_DATATYPE_STRING,
	XP_DATATYPE_BOOL,
	XP_DATATYPE_BITS,
	XP_DATATYPE_INT
} XPStandardDatatypeID;

struct XplaneTrans {
	uint16_t	canasId;
	char*		xplaneId;
	int			xpDataType;
	int			xpTimesSecond;
	float		conversionFactor;
	int			stringSize;
};

struct XplanePlanes {
	char* name;
	XplaneTrans canasXplaneTable[MAX_XP_CAN_ITEMS];
};

//
// standard 172
// sim/aircraft/view/acf_author="Laminar Research - dmax3d.com"
// sim/aircraft/view/acf_descip="Cessna 172 SP Skyhawk - 180HP"
// FUELL	laminar/c172/fuel/fuel_quantity_L
// FUELR	laminar/c172/fuel/fuel_quantity_R
// AMPS		laminar/c172/electrical/battery_amps
// VACUEM	sim/cockpit/misc/vacuum
// OIL_P	sim/cockpit2/indicators/oil_pressure_psi[0]
// OIL_T	sim/cockpit2/indicators/oil_temperature_deg_C[0]
// EGT		sim/cockpit2/indicators/EGT_deg_C[0]
//
// standard 172
// sim/aircraft/view/acf_author="Airfoillabs"
// sim/aircraft/view/acf_descip="Airfoillabs Cessna 172SP"
// FUELL	laminar/c172/fuel/fuel_quantity_L
// FUELR	laminar/c172/fuel/fuel_quantity_R
// AMPS		laminar/c172/electrical/battery_amps
// VACUEM	sim/cockpit/misc/vacuum
// OIL_P	sim/cockpit2/indicators/oil_pressure_psi[0]
// OIL_T	sim/cockpit2/indicators/oil_temperature_deg_C[0]
// EGT		172/instruments/uni_EGT
// lights   172/lights/int/int_radio_panel_lt

// TODO: make 2 tables one for master and one for instruments without text strings
// #ifdef ICAN_MASTER

// todo : make array for multiple versions of plane
static XplaneTrans getPlaneName = { CANAS_NOD_PLANE_NAME,"sim/aircraft/view/acf_descrip",XP_DATATYPE_STRING,2,1,260 };

static XplanePlanes suportedPlanes[]{
	{"Cessna 172 SP Skyhawk - 180HP",
		{
			{CANAS_NOD_DEF_172_FUEL_L,"laminar/c172/fuel/fuel_quantity_L",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_FUEL_R,"laminar/c172/fuel/fuel_quantity_R",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_EGT,"sim/cockpit2/indicators/EGT_deg_C[0]",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_FUEL_FLOW,"sim/cockpit2/engine/indicators/fuel_flow_kg_sec[0]",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_BAT_AMPS,"laminar/c172/electrical/battery_amps",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_SUCTION,"sim/cockpit/misc/vacuum",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_OIL_TMP,"sim/cockpit2/indicators/oil_temperature_deg_C[0]",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_OIL_PRES,"sim/cockpit2/indicators/oil_pressure_psi[0]",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_USR_INSTRUMENT_LIGHT_INTENSITY,"sim/cockpit/electrical/instrument_brightness",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_USR_AVIONICS_ON,"172/ip_base/b_avionics_master",XP_DATATYPE_FLOAT,5,1},
			{0,"",0}
		}
	},

	{"Airfoillabs Cessna 172SP",
		{
			{CANAS_NOD_DEF_172_FUEL_L,"172/instruments/uni_fuel_L_x",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_FUEL_R,"172/instruments/uni_fuel_R_x",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_EGT,"172/instruments/uni_EGT",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_FUEL_FLOW,"172/instruments/uni_FF",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_BAT_AMPS,"172/instruments/uni_bat_amps",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_SUCTION,"172/instruments/uni_suction",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_OIL_TMP,"172/instruments/uni_oil_F",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_DEF_172_OIL_PRES,"172/instruments/uni_oil_pres",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_USR_INSTRUMENT_LIGHT_INTENSITY,"172/lights/int/int_radio_panel_lt",XP_DATATYPE_FLOAT,2,1},
			{CANAS_NOD_USR_AVIONICS_ON,"sim/cockpit/electrical/battery_on",XP_DATATYPE_FLOAT,5,1},
			{0,"",0}
		}
	}
};
//==================================================================================================
//==================================================================================================
class XpPlaneInfo {
public:

	XpPlaneInfo();
	~XpPlaneInfo();

	static char* fromCan2Xplane(uint16_t canID);
	static  XplaneTrans* fromCan2XplaneElement(uint16_t canID);
	static  XplaneTrans* getName();
	static int findPlane(char* planeName);
private:
	static int _currentPlane;
};
#endif
