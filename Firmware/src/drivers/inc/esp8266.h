#pragma once
#ifndef __ESP8266_H
#define __ESP8266_H

#include <stm32f1xx_hal.h>
#include "ssd1306.h"

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
	char* FindTimeFromServer(const char* url);
protected:
	UART_HandleTypeDef* usartHandle;
	SSD1306* display; 
private:
	uint8_t buf[ESP_RCV_BUF_SIZE];
	char send_buf[80];
	static char* search_buffer(char* haystack, uint16_t haystacklen, char* needle, uint16_t needlelen);
};




#endif /* __ESP8266_H */