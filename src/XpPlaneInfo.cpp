#include "XpPlaneInfo.h"
#include "xpudp_debug.h"
#include <stdlib.h>
#include <stdio.h>
#include "IniFile.h"

static const char *TAG = "XpPlaneInfo";
#include "esp_log.h"

//int XpPlaneInfo::_currentPlane = -1;
bool XpPlaneInfo::_PlaneLoaded = false;

char* XpPlaneInfo::_iniFile = NULL;
//QList<XpPlaneInfo::_planeInfo *> XpPlaneInfo::_planeInfoItems;
QList<XplaneTrans*>XpPlaneInfo::_planeTranslations;

//==================================================================================================
//==================================================================================================
char* XpPlaneInfo::fromCan2Xplane(uint16_t canID)
{
	int i = 0;

	if (!_PlaneLoaded)
		return NULL;

	for (i = 0; i < _planeTranslations.length(); i++)
	{
		if (_planeTranslations[i]->canasId == canID)
			break;
	}

	if (i < _planeTranslations.length())
	{
		return _planeTranslations[i]->xplaneId;
	}

	ESP_LOGD(TAG, "No record found for %d", canID);
	return NULL;
}
//==================================================================================================
//==================================================================================================
XplaneTrans* XpPlaneInfo::fromCan2XplaneElement(uint16_t canID)
{
	int i = 0;

	if (!_PlaneLoaded)
		return NULL;

	for (i = 0; i < _planeTranslations.length() && (_planeTranslations[i]->canasId != canID); i++);

	if (i < _planeTranslations.length())
	{
		return _planeTranslations[i];
	}

	ESP_LOGD(TAG, "No record found for %d", canID);

	return NULL;
}

//==================================================================================================
//==================================================================================================
int XpPlaneInfo::findPlane(char * planeName)
{
	int i;
	/*for (i = 0; i < _planeInfoItems.size(); i++)
	{
		if (strcmp(_planeInfoItems[i]->name, planeName) == 0)
		{
			ESP_LOGD(TAG, "record found for [%s]", planeName);
			break;
		}
	}*/

	//if (i < _planeInfoItems.size())
	//{
	return (_readIniFile(planeName));
	/*}
	else
	{
		ESP_LOGE(TAG, "record not found for [%s]", planeName);
		return -10;
	}*/
}
//==================================================================================================
//==================================================================================================
int XpPlaneInfo::setIniFile(const char * fileName)
{
	char fullPath[256] = "/sdcard/";

	strcat(fullPath, fileName);

	if (fileName != NULL)
	{
		if (_iniFile != NULL)
			free(_iniFile);

		_iniFile = (char*)malloc(strlen(fullPath) + 1);
		strcpy(_iniFile, fullPath);
		return (0);
	}
	return -1;
}
//==================================================================================================
// read new info for a plane after plane name is set
//==================================================================================================
int XpPlaneInfo::_readIniFile(char* planeName)
{
	int result = -2;
	char foundBuff[128];
	const size_t bufferLen = 256;
	char buffer[bufferLen];

	if (planeName == NULL)
		return -1;

	//FILE *file = fopen(_iniFile, "rb");

	ESP_LOGD(TAG, "start parse planefile :%s", _iniFile);
	IniFile ini(_iniFile, "rb");
	IniFileState state;

	if (!ini.open())
	{
		ESP_LOGE(TAG, "planefile does not exist");
		// Cannot do anything else
		return -1;
	}

	result = -3;
	// first clean lists

	XplaneTrans *curRec;
	_PlaneLoaded = false;

	for (int i = 0; _planeTranslations.size(); i++)
	{
		curRec = _planeTranslations[i];
		free(curRec->xplaneId);
		delete curRec;
	}
	_planeTranslations.clear();

	int added = 0;
	// loop thru items and store

	ESP_LOGV(TAG, "planefile opened");
	// find first item
	while (!ini.getValue(planeName, "item", buffer, bufferLen, state));
	ESP_LOGV(TAG, "inifile error is %d", ini.getError());

	while (ini.getError() == IniFile::errorNoError)
	{
		ESP_LOGV(TAG, "item found :%s:", buffer);

		char *cp = IniFile::skipWhiteSpace(buffer);

		// proces the reacord add it to struct
		curRec = new XplaneTrans;
		// get canid
		if ((cp = _getNext(cp, ',', foundBuff)) != NULL)
		{
			curRec->canasId = atoi(foundBuff);
			ESP_LOGV(TAG, "canasid= %d", curRec->canasId);
		}
		else { result = -4; break; }
		// get xplaneId
		if ((cp = _getNext(cp, ',', foundBuff)) != NULL)
		{
			curRec->xplaneId = (char*)malloc(strlen(foundBuff) + 1);
			strcpy(curRec->xplaneId, trimQuotes(foundBuff));
			ESP_LOGV(TAG, "xplaneid= %s", curRec->xplaneId);
		}
		else { result = -4; break; }
		//get datatype
		if ((cp = _getNext(cp, ',', foundBuff)) != NULL)
		{
			char* strItem = trimQuotes(foundBuff);
			ESP_LOGV(TAG, "search for type :%s:", strItem);
			if (strcasecmp("FLOAT", strItem) == 0)
			{
				curRec->xpDataType = XP_DATATYPE_FLOAT;
				ESP_LOGV(TAG, "found FLOAT");
			}
			else if (strcasecmp("STRING", strItem) == 0)
			{
				curRec->xpDataType = XP_DATATYPE_STRING;
				ESP_LOGV(TAG, "found STRING");
			}
			else if (strcasecmp("INT", strItem) == 0)
			{
				curRec->xpDataType = XP_DATATYPE_INT;
			}
			else if (strcasecmp("BITS", strItem) == 0)
			{
				curRec->xpDataType = XP_DATATYPE_BITS;
			}
			else if (strcasecmp("BOOL", strItem) == 0)
			{
				curRec->xpDataType = XP_DATATYPE_BOOL;
			}
			else
			{
				result - 5;
				break;
			}
			ESP_LOGV(TAG, "xpDataType= %d", curRec->xpDataType);
		}
		else { result = -4; break; }
		// get times/second
		if ((cp = _getNext(cp, ',', foundBuff)) != NULL)
		{
			curRec->xpTimesSecond = atoi(foundBuff);
			ESP_LOGV(TAG, "xpTimesSecond= %d", curRec->xpTimesSecond);
		}
		else { result = -4; break; }

		// get conversion factor
		if ((cp = _getNext(cp, ',', foundBuff)) != NULL)
		{
			curRec->conversionFactor = atof(foundBuff);
			ESP_LOGV(TAG, "conversionFactor= %f", curRec->conversionFactor);
		}
		else { result = -4; break; }
		// optional get string size if string type
		if (curRec->xpDataType == XP_DATATYPE_STRING)
			if ((cp = _getNext(cp, ',', foundBuff)) != NULL)
			{
				curRec->stringSize = atoi(foundBuff);
				ESP_LOGV(TAG, "stringSize= %f", curRec->stringSize);
			}
			else { result = -4; break; }

		ESP_LOGV(TAG, "New rec |%d|%s|%d|%d|%f|%d",
			curRec->canasId, curRec->xplaneId, curRec->xpDataType,
			curRec->xpTimesSecond, curRec->conversionFactor, curRec->stringSize);

		// store
		_planeTranslations.push_back(curRec);
		added++;
		result = 0;

		while (!ini.getValue(planeName, "item", buffer, bufferLen, state));
	}

	ESP_LOGD(TAG, "Added %d records", added);
	_PlaneLoaded = true;
	ini.close();

	//	fclose(file);
	//}
	//else result = -11;
	ESP_LOGD(TAG, "end parse planefile");
	return result;
}
//==================================================================================================
char* XpPlaneInfo::_getNext(char* buffer, char terminator, char*  newVal)
{
	int newsize;

	ESP_LOGV(TAG, "search for :%c: in :%s:", terminator, buffer);

	char *ep = strchr(buffer, terminator);
	if (ep == NULL && (strlen(buffer) == 0))
	{
		ESP_LOGV(TAG, "terminator not found :%c: in :%s:", terminator, buffer);
		return NULL;
	}

	if (ep == NULL)
	{
		strcpy(newVal, buffer);
		ep = buffer + strlen(buffer);
	}
	else
	{
		newsize = ep - buffer;
		strncpy(newVal, buffer, newsize);
		newVal[newsize] = '\0';
		ESP_LOGV(TAG, "new item found :%s:", newVal);
		ep++;
	}
	ESP_LOGV(TAG, "new buffer :%s:", ep);
	return ep;
}
char* XpPlaneInfo::trimQuotes(char* inStr)
{
	char* tmpStr;

	int i, j;
	for (i = 0; i < strlen(inStr) && inStr[i] != '"'; i++);
	for (j = strlen(inStr); j >= 0 && inStr[j] != '"'; j--);
	inStr[j] = '\0';

	tmpStr = inStr + i;
	tmpStr++;
	ESP_LOGD(TAG, "trimed to :%s: %d %d", tmpStr, i, j);

	return tmpStr;
}
//==================================================================================================
// read all available plane names from file.
//==================================================================================================
//int XpPlaneInfo::_scanPlanes()
//{
//	FILE* file;
//	int result = -1;
//	char buffer[256];
//	_planeInfo* curRec;
//
//	file = fopen(_iniFile, "rb");
//	if (file != NULL)
//	{
//		// clean content of old list
//		for (int i = 0; _planeTranslations.size(); i++)
//		{
//			curRec = _planeTranslations[i];
//			free(curRec->name);
//			delete curRec;
//		}
//		_planeTranslations.clear();
//
//		result = -2;
//		while (fgets(buffer, 255, file) != NULL)
//		{
//			char *cp = IniFile::skipWhiteSpace(buffer);
//			if (cp[0] == '[')
//			{
//				cp++;
//				// header found read header;
//				char *ep = strchr(cp, ']');
//				*ep = '\0';
//				IniFile::removeTrailingWhiteSpace(cp);
//				_planeInfo* planeInfo = new _planeInfo;
//				planeInfo->filePos = ftell(file);
//				planeInfo->name = (char*)malloc(strlen(cp) + 1);
//				strcpy(planeInfo->name, cp);
//				_planeInfoItems.push_back(planeInfo);
//				result = 0;
//				ESP_LOGD(TAG, "Planerecord found [%s]", cp);
//			}
//		}
//	}
//
//	fclose(file);
//
//	return result;
//}
//==================================================================================================
//==================================================================================================
int XpPlaneInfo::_findPlaneItem(uint16_t canID)
{
	int i = -1;
	for (i = 0; i < _planeTranslations.length(); i++)
	{
		if (_planeTranslations[i]->canasId == canID)
		{
			return i;
		}
	}

	return -1;
}