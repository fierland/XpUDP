#ifndef __XPUDPTEST_H_
#define __XPUDPTEST_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <stdint.h>
#include <QList.h>

#include "GenericDefs.h"
#include <esp_task_wdt.h>
#include <limits.h>

typedef struct xUDPtestInfo {
	uint16_t canId;
	float minVal;
	float maxVal;
	float deltaVal;
	int interval;
	float startVal;
};

class XpUDPtest {
public:
#define MAX_TEST_ITEMS 10

	xUDPtestInfo _testValueList[MAX_TEST_ITEMS] = {
	{ 201,  0, 26 ,-0.01,10, 20},
	{ 202,  0, 26 ,-0.01,10, 15},
	{ 203,  0,400 ,   0,10,200},
	{ 204,  0, 19 ,   0,10,  3},
	{ 205,-60, 60 ,   0,10, 40},
	{ 206, 75,245 ,   0,10,150},
	{ 207,  0,115 ,   0,10,100},
	{ 208,  3,  7 ,   0,10,  5},
	{ 209,  0,  1 ,-100,10, 1},
	{1800,  0,100 ,   0,10, 60}
	};

	XpUDPtest();
	~XpUDPtest();
	void testMainLoop();
	void start(int core);
	void stop();

protected:

	int _testInfoItems = MAX_TEST_ITEMS;

	typedef struct xUDPtestListItem {
		uint16_t	canId = 0;
		float		curentvalue = 0;
		long		ts = 0;
		bool		active = true;
		bool		send = true;
		xUDPtestInfo* info = NULL;
	};

	QList<xUDPtestListItem*> _xUDPtestList;

	TaskHandle_t xTaskTest = NULL;

	void unRegisterDataRef(uint16_t canId);
	void registerDataRef(uint16_t canId, bool send);
	int _findTestInfo(uint16_t canId);

	int _findInlist(uint16_t canId);
};

#endif
