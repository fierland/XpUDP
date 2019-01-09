static const char *TAG = "XpUDPtest";

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include <stdlib.h>

#include <esp_task_wdt.h>
#include "esp32-hal.h"
#include <time.h>

#include "XpUDPtest.h"

#include "xpudp_debug.h"
#include "esp_log.h"

//==================================================================================================
//--------------------------------------------------------------------------------------------------
extern "C" void taskXupdTest(void* parameter)
{
	XpUDPtest* xpudp;

	ESP_LOGD(TAG, "Task xUdpTest started ");

	xpudp = (XpUDPtest*)parameter;
	vTaskDelay(500);

	for (;;)
	{
		esp_task_wdt_reset();
		xpudp->testMainLoop();
		vTaskDelay(200);
	}
}

//==================================================================================================
//==================================================================================================

XpUDPtest::XpUDPtest()
{}

//==================================================================================================
//==================================================================================================
XpUDPtest::~XpUDPtest()
{}
//==================================================================================================
//==================================================================================================
void XpUDPtest::testMainLoop()
{
	queueDataSetItem	_DataSetItem;
	queueDataItem		_DataItem;
	int i;
	float oldVal;
	long ts = millis();

	// check RREF queue;
	esp_task_wdt_reset();

	while (xQueueReceive(xQueueDataSet, &_DataSetItem, 0) == pdTRUE)
	{
		if (_DataSetItem.interval == 0)
			unRegisterDataRef(_DataSetItem.canId);
		else
			registerDataRef(_DataSetItem.canId, _DataSetItem.send);
	};

	// check DREFqueue
	esp_task_wdt_reset();

	while (xQueueReceive(xQueueDREF, &_DataItem, 0) == pdTRUE)
	{
		ESP_LOGI(TAG, "Got new value for DREF=%d value=%f", _DataItem.canId,
			_DataItem.data.container.FLOAT);
		if (_findInlist(_DataItem.canId) == -1)
		{
			ESP_LOGE(TAG, "!! CanId is not registered before !!");
		}
	};
	// check dataitem queu

	esp_task_wdt_reset();
	for (i = 0; i < _xUDPtestList.length(); i++)
	{
		//ESP_LOGV(TAG, "check [%d] status %s active is %s and has %sinfo", _xUDPtestList[i]->canId,
		//	(_xUDPtestList[i]->active) ? "" : "NOT ", _xUDPtestList[i]->send ? "sending" : "receiving",
		//	(_xUDPtestList[i]->info != NULL) ? "" : "NO ");

		if (_xUDPtestList[i]->active && !_xUDPtestList[i]->send && _xUDPtestList[i]->info != NULL)
		{
			//ESP_LOGV(TAG, "interval [%d] current wait %d", _xUDPtestList[i]->info->interval * 1000,
			//	(ts - _xUDPtestList[i]->ts));
			oldVal = _xUDPtestList[i]->curentvalue;

			//ESP_LOGV(TAG, "old value = %f", _xUDPtestList[i]->curentvalue);
			if ((_xUDPtestList[i]->info->interval * 1000) < (ts - _xUDPtestList[i]->ts))
			{
				_xUDPtestList[i]->ts = ts;
				// change value
				if (_xUDPtestList[i]->info->deltaVal != -100)
				{
					if (!_xUDPtestList[i]->info->deltaVal)
					{
						float delta;
						float diffValue = _xUDPtestList[i]->info->maxVal - _xUDPtestList[i]->info->minVal;
						int dir;

						delta = (float)(rand() % ((int)diffValue * 10));
						delta /= 100;
						dir = rand() % 100;
						if (dir < 50) delta *= -1;

						_xUDPtestList[i]->curentvalue += delta;

						if (_xUDPtestList[i]->curentvalue < _xUDPtestList[i]->info->minVal)
							_xUDPtestList[i]->curentvalue = _xUDPtestList[i]->info->minVal;

						if (_xUDPtestList[i]->curentvalue > _xUDPtestList[i]->info->maxVal)
							_xUDPtestList[i]->curentvalue = _xUDPtestList[i]->info->maxVal;

						ESP_LOGV(TAG, "delta value = (%f) %f %d new=%f", diffValue, delta, dir, _xUDPtestList[i]->curentvalue);
						delay(10);
					}
					else
					{
						_xUDPtestList[i]->curentvalue += _xUDPtestList[i]->info->deltaVal;

						if (_xUDPtestList[i]->curentvalue < _xUDPtestList[i]->info->minVal)
							_xUDPtestList[i]->curentvalue = _xUDPtestList[i]->info->maxVal;

						if (_xUDPtestList[i]->curentvalue > _xUDPtestList[i]->info->maxVal)
							_xUDPtestList[i]->curentvalue = _xUDPtestList[i]->info->minVal;
					}
				}
				ESP_LOGV(TAG, "[%d] value = (%f) %f", _xUDPtestList[i]->canId, oldVal, _xUDPtestList[i]->curentvalue);
				//send to queue
				_DataItem.canId = _xUDPtestList[i]->canId;

				switch (_xUDPtestList[i]->info->dataType)
				{
				case CANAS_DATATYPE_FLOAT:
					_DataItem.data.container.FLOAT = _xUDPtestList[i]->curentvalue;
					_DataItem.data.type = CANAS_DATATYPE_FLOAT;
					break;
				case CANAS_DATATYPE_UCHAR:
					_DataItem.data.type = CANAS_DATATYPE_UCHAR;
					_DataItem.data.container.UCHAR = (unsigned char)_xUDPtestList[i]->curentvalue;
					break;
				}

				if (xQueueSendToBack(xQueueRREF, &_DataItem, 10) != pdPASS)
				{
					ESP_LOGE(TAG, "Error posting RREF to queue");
				}
			}
		}
	}
}
//==================================================================================================
//==================================================================================================
void XpUDPtest::start(int core)
{
	// starting main tasks
	ESP_LOGV(TAG, "Startting main tasks");

	xTaskCreatePinnedToCore(taskXupdTest, "taskXPtest", 15000, this, 4, &xTaskTest, core);

	esp_task_wdt_add(xTaskTest);

	ESP_LOGV(TAG, "Task XupdTest started");
}
//==================================================================================================
//==================================================================================================
void XpUDPtest::stop()
{
	vTaskDelete(xTaskTest);
	ESP_LOGV(TAG, "Task XupdTest stoped");
}
//==================================================================================================
//==================================================================================================
void XpUDPtest::unRegisterDataRef(uint16_t canId)
{
	int i = _findInlist(canId);
	if (i > -1 && _xUDPtestList[i]->active)
	{
		_xUDPtestList[i]->active = false;
		ESP_LOGI(TAG, "Unregistering CanId [%d]", canId);
	}
	else
	{
		ESP_LOGE(TAG, "!!Unregistering CanId [%d] is not active !!", canId);
	}
}
void XpUDPtest::registerDataRef(uint16_t canId, bool send)
{
	int i = _findInlist(canId);
	if (i > -1 && _xUDPtestList[i]->active)
	{
		ESP_LOGE(TAG, "!!Registering CanId [%d] is already active !!", canId);
	}
	else if (i > -1 && !_xUDPtestList[i]->active)
	{
		_xUDPtestList[i]->active = true;
		ESP_LOGI(TAG, "Re-Registering CanId [%d]", canId);
	}
	else
	{
		ESP_LOGI(TAG, "Registering CanId [%d]", canId);
		xUDPtestListItem* newItem = new xUDPtestListItem;
		newItem->canId = canId;

		int j = _findTestInfo(canId);

		if (j < 0)
		{
			ESP_LOGE(TAG, "!!Unknown CanId [%d] !!", canId);
		}
		else
		{
			newItem->info = &(_testValueList[j]);
			newItem->curentvalue = _testValueList[j].startVal;
			ESP_LOGD(TAG, "set value to=%f", _testValueList[j].startVal);
		}
		newItem->send = send;
		_xUDPtestList.push_back(newItem);
	}
}
//==================================================================================================
//==================================================================================================

int XpUDPtest::_findTestInfo(uint16_t canId)
{
	int res = -1;
	for (int i = 0; i < MAX_TEST_ITEMS; i++)
	{
		ESP_LOGD(TAG, "TestInfo testing [%d] -> [%d]", canId, _testValueList[i].canId);
		if (_testValueList[i].canId == canId)
		{
			res = i;
			ESP_LOGV(TAG, "item found %d %f %f %f %d %f",
				_testValueList[i].canId,
				_testValueList[i].minVal,
				_testValueList[i].maxVal,
				_testValueList[i].deltaVal,
				_testValueList[i].interval,
				_testValueList[i].startVal
			);

			break;
		}
	};
	return res;
}

//==================================================================================================
//==================================================================================================
int XpUDPtest::_findInlist(uint16_t canId)
{
	int i = -1;
	for (i = 0; i < _xUDPtestList.length(); i++)
	{
		if (_xUDPtestList[i]->canId == canId)
		{
			return i;
		}
	}

	return -1;
}