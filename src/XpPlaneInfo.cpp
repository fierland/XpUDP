#include "XpPlaneInfo.h"
#include "xpudp_debug.h"
#include <stdlib.h>
#include <stdio.h>
#include "IniFile.h"

static const char *TAG = "XpPlaneInfo";
#include "esp_log.h"

int XpPlaneInfo::_currentPlane = -1;

char* XpPlaneInfo::_iniFile = NULL;
QList<XpPlaneInfo::_planeInfo *> XpPlaneInfo::_planeInfoItems;
QList<XplaneTrans*>XpPlaneInfo::_planeTranslations;

//==================================================================================================
//==================================================================================================
char* XpPlaneInfo::fromCan2Xplane(uint16_t canID)
{
	int i = 0;

	if (_currentPlane != -1)
	{
		//while (suportedPlanes[_currentPlane].canasXplaneTable[i].canasId != 0 && suportedPlanes[_currentPlane].canasXplaneTable[i].canasId != canID)i++;
		if (_findPlane(canID) != -1)
		{
			return _planeTranslations[i]->xplaneId;
		}
	}

	ESP_LOGD(TAG, "No record found for %d", canID)
		return NULL;
}
//==================================================================================================
//==================================================================================================
XplaneTrans* XpPlaneInfo::fromCan2XplaneElement(uint16_t canID)
{
	int i = 0;

	if (_currentPlane != -1)
	{
		//while (suportedPlanes[_currentPlane].canasXplaneTable[i].canasId != 0 && suportedPlanes[_currentPlane].canasXplaneTable[i].canasId != canID)i++;
		if (_findPlane(canID) != -1)
		{
			return _planeTranslations[i];
		}
	}

	ESP_LOGD(TAG, "No record found for %d", canID)

		return NULL;
}

//==================================================================================================
//==================================================================================================
int XpPlaneInfo::findPlane(char * planeName)
{
	int i;
	for (i = 0; i < _planeInfoItems.size(); i++)
	{
		if (strcmp(_planeInfoItems[i]->name, planeName) == 0)
		{
			ESP_LOGD(TAG, "record found for [%s]", planeName);
		}
	}

	if (i < _planeInfoItems.size())
	{
		return (_readIniFile(_planeInfoItems[i]));
	}
	else
	{
		ESP_LOGE(TAG, "record not found for [%s]", planeName);
		return -10;
	}
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
		return (_scanPlanes());
	}
	return -1;
}
//==================================================================================================
// read new info for a plane after plane name is set
//==================================================================================================
int XpPlaneInfo::_readIniFile(_planeInfo* planeInfo)
{
	int result = -2;
	char buffer[256];
	char foundBuff[128];

	if (planeInfo == NULL)
		return -1;

	FILE *file = fopen(_iniFile, "rb");
	if (file != NULL)
	{
		if (fseek(file, planeInfo->filePos, SEEK_SET) == 0)
		{
			result = -3;
			// first clean lists

			XplaneTrans *curRec;

			for (int i = 0; _planeTranslations.size(); i++)
			{
				curRec = _planeTranslations[i];
				free(curRec->xplaneId);
				delete curRec;
			}
			_planeTranslations.clear();

			int added = 0;
			// loop thru items and store
			while (fgets(buffer, 255, file) != NULL)
			{
				char *cp = IniFile::skipWhiteSpace(buffer);
				if (cp[0] == '[')
				{
					// new header reached so stop reading;
					break;
				}
				else
				{
					// proces the reacord add it to struct
					curRec = new XplaneTrans;
					// get canid
					if (_getNext(cp, ',', foundBuff))
					{
						curRec->canasId = atoi(foundBuff);
					}
					else { result = -4; break; }
					// get xplaneId
					if (_getNext(cp, ',', foundBuff))
					{
						curRec->xplaneId = (char*)malloc(strlen(foundBuff) + 1);
						strcpy(curRec->xplaneId, foundBuff);
					}
					else { result = -4; break; }
					//get datatype
					if (_getNext(cp, ',', foundBuff))
					{
						if (strcasecmp("FLOAT", foundBuff))
						{
							curRec->xpDataType = XP_DATATYPE_FLOAT;
						}
						else if (strcasecmp("STRING", foundBuff))
						{
							curRec->xpDataType = XP_DATATYPE_STRING;
						}
						else if (strcasecmp("INT", foundBuff))
						{
							curRec->xpDataType = XP_DATATYPE_INT;
						}
						else if (strcasecmp("BITS", foundBuff))
						{
							curRec->xpDataType = XP_DATATYPE_BITS;
						}
						else if (strcasecmp("BOOL", foundBuff))
						{
							curRec->xpDataType = XP_DATATYPE_BOOL;
						}
						else
						{
							result - 5;
							break;
						}
					}
					else { result = -4; break; }
					// get times/second
					if (_getNext(cp, ',', foundBuff))
					{
						curRec->xpTimesSecond = atoi(foundBuff);
					}
					else { result = -4; break; }

					// get conversion factor
					if (_getNext(cp, ',', foundBuff))
					{
						curRec->conversionFactor = atof(foundBuff);
					}
					else { result = -4; break; }
					// optional get string size if string type
					if (curRec->xpDataType == XP_DATATYPE_STRING && _getNext(cp, ',', foundBuff))
					{
						curRec->stringSize = atof(foundBuff);
					}
					else { result = -4; break; }

					ESP_LOGV(TAG, "New rec |%d|%s|%d|%d|%f|%d",
						curRec->canasId, curRec->xplaneId, curRec->xpDataType,
						curRec->xpTimesSecond, curRec->conversionFactor, curRec->stringSize);

					// store
					_planeTranslations.push_back(curRec);
					added++;
					result = 0;
				}
				ESP_LOGD(TAG, "Added %d records", added);
			}
		}

		fclose(file);
	}
	else result = -11;

	return result;
}
//==================================================================================================
int XpPlaneInfo::_getNext(char* buffer, char terminator, char*  newVal)
{
	int newsize;

	char *ep = strchr(buffer, terminator);
	if (ep == NULL)
	{
		return -1;
	}

	newsize = ep - buffer;

	strncpy(newVal, buffer, newsize);

	buffer = ep;
	buffer++;

	return 0;
}
//==================================================================================================
// read all available plane names from file.
//==================================================================================================
int XpPlaneInfo::_scanPlanes()
{
	FILE* file;
	int result = -1;
	char buffer[256];
	_planeInfo* curRec;

	file = fopen(_iniFile, "rb");
	if (file != NULL)
	{
		// clean content of old list
		for (int i = 0; _planeInfoItems.size(); i++)
		{
			curRec = _planeInfoItems[i];
			free(curRec->name);
			delete curRec;
		}
		_planeInfoItems.clear();

		result = -2;
		while (fgets(buffer, 255, file) != NULL)
		{
			char *cp = IniFile::skipWhiteSpace(buffer);
			if (cp[0] == '[')
			{
				cp++;
				// header found read header;
				char *ep = strchr(cp, ']');
				*ep = '\0';
				IniFile::removeTrailingWhiteSpace(cp);
				_planeInfo* planeInfo = new _planeInfo;
				planeInfo->filePos = ftell(file);
				planeInfo->name = (char*)malloc(strlen(cp) + 1);
				strcpy(planeInfo->name, cp);
				_planeInfoItems.push_back(planeInfo);
				result = 0;
				ESP_LOGD(TAG, "Planerecord found [%s]", cp);
			}
		}
	}

	fclose(file);

	return result;
}
//==================================================================================================
//==================================================================================================
int XpPlaneInfo::_findPlane(uint16_t canID)
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