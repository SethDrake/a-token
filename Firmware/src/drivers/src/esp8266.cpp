
#include "src/drivers/inc/esp8266.h"
#include <cstring>


ESP8266::ESP8266()
{
	this->usartHandle = NULL;
	memset(buf, 0x00, ESP_RCV_BUF_SIZE);
}

ESP8266::~ESP8266()
{
}

void ESP8266::initModule(UART_HandleTypeDef* usartHandle, SSD1306* display)
{
	this->usartHandle = usartHandle;
	this->display = display;
}

void ESP8266::OnRxInterrupt()
{
	HAL_UART_IRQHandler(this->usartHandle);
}

bool ESP8266::SendCommand(const char* cmd)
{
	memset(buf, 0x00, ESP_RCV_BUF_SIZE);
	const HAL_StatusTypeDef status = HAL_UART_Transmit(this->usartHandle, (uint8_t*)cmd, strlen(cmd), 500);
	return status == HAL_OK;
}

bool ESP8266::SendCommandWithResponse(const char* cmd, const char* resp, const uint32_t Timeout)
{
	if (!this->SendCommand(cmd))
	{
		return false;	
	}
	HAL_UART_Receive(this->usartHandle, buf, ESP_RCV_BUF_SIZE, Timeout);
	char* str = search_buffer((char*)buf, 64, (char*)resp, strlen(resp));
	if (!str)
	{
		return false;		
	}
	return true;
}

bool ESP8266::IsConnected(void)
{
	return SendCommandWithResponse("AT+CIFSR\r\n", "+CIFSR:APIP,", 500);
}

bool ESP8266::ConfigureModule(const char* ssid, const char* password)
{
	if (!SendCommandWithResponse("AT\r\n", "OK\r\n"))
	{
		return false;		
	}

	//Config ESP8266
	if (!SendCommandWithResponse("ATE0\r\n", "OK\r\n")) return false;
	if (!SendCommandWithResponse("AT+CWMODE=1\r\n", "OK\r\n")) return false;
	const bool isConnected = IsConnected();
	if (!isConnected)
	{
		if (!SendCommandWithResponse("AT+CWQAP\r\n", "OK\r\n", 500)) return false;		
		if (!SendCommandWithResponse("AT+CWJAP=\"GAMMA_14N\",\"Access117\"\r\n", "OK\r\n", 10000)) return false;
		if (!IsConnected())
		{
			return false;	
		}
	}
	return true;
}

char* ESP8266::FindTimeFromServer(const char* url)
{
	if (!IsConnected())
	{
		return NULL;
	}
	memset(send_buf, 0x00, 80);
	sprintf(send_buf, "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", url);
	if (!SendCommandWithResponse((const char*)send_buf, "CONNECT\r\n", 500)) return NULL;

	if (!SendCommandWithResponse("AT+CIPSEND=15\r\n", "OK\r\n>")) return NULL;
	if (!SendCommandWithResponse("HEAD HTTP/1.1\r\n", "OK\r\n"), 500) return NULL;
	return NULL;
}

char* ESP8266::search_buffer(char* haystack, uint16_t haystacklen, char* needle, uint16_t needlelen)
{ 
	int searchlen = haystacklen - needlelen + 1;
	for (; searchlen-- > 0; haystack++)
		if (!memcmp(haystack, needle, needlelen))
			return haystack;
	return NULL;
}
