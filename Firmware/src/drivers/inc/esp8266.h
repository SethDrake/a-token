#pragma once
#ifndef __ESP8266_H
#define __ESP8266_H

#include <stm32f1xx_hal.h>
#include "ssd1306.h"
#include <time.h>
#include <cstring>

#define ESP_RCV_BUF_SIZE 255

class ESP8266 {
public:
	ESP8266();
	~ESP8266();
	void initModule(UART_HandleTypeDef* usartHandle, SSD1306* display);
	void OnRxInterrupt(void);
	bool SendCommand(const char *cmd);
	bool SendCommandWithResponse(const char* cmd, const char* resp, uint32_t Timeout = 250);
	bool ConfigureModule(const char *ssid, const char *password);
	bool IsConnected(void);
	time_t FindTimeFromServer(const char* url);
protected:
	UART_HandleTypeDef* usartHandle;
	SSD1306* display; 
	int8_t StrMonthToInt(const char* month);
private:
	uint8_t buf[ESP_RCV_BUF_SIZE];
	char send_buf[80];
	char at_version[10];
	char sdk_version[10];
};




#endif /* __ESP8266_H */