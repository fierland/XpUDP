#ifndef __GENERICDEFS_H_
#define __GENERICDEFS_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

extern QueueHandle_t xQueueRREF;
extern QueueHandle_t xQueueDREF;
extern QueueHandle_t xQueueDataSet;
extern QueueHandle_t xQueueStatus;

typedef struct queueDataSetItem {
	uint16_t	canId;
	int			interval = 1;
	bool		send = false;
};

typedef struct queueDataItem {
	uint16_t	canId;
	float		value;
};

#endif
