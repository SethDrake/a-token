#pragma once
#ifndef __OBJECTS_H_
#define __OBJECTS_H_
#include "drivers/inc/ssd1306.h"
#include "drivers/inc/esp8266.h"

typedef enum
{ 
	LOADING = 0,
	ACTIVE,
	SLEEP
} SYSTEM_MODE;

typedef enum
{ 
	WIFI_LOADING = 0,
	WIFI_CONNECTED,
	WIFI_ERROR
} WIFI_STATE;

extern SYSTEM_MODE systemMode;
extern WIFI_STATE wifiState;

extern SSD1306 display;
extern ESP8266 wifi;

extern DMA_HandleTypeDef i2cDmaTx;
extern TIM_HandleTypeDef htim1;
extern PCD_HandleTypeDef hpcd_USB_FS;

#endif //__OBJECTS_H_