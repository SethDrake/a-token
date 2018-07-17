
#include "src/drivers/inc/esp8266.h"


ESP8266::ESP8266(): display(nullptr)
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
	HAL_UART_Receive(this->usartHandle, (uint8_t*)buf, ESP_RCV_BUF_SIZE, Timeout);
	char* exists = strstr((char*)buf, resp);
	if (!exists)
	{
		return false;		
	}
	return true;
}

bool ESP8266::IsConnected(void)
{
	return SendCommandWithResponse("AT+CIFSR\r\n", "+CIFSR:STAIP,", 500);
}

bool ESP8266::ConfigureModule(const char* ssid, const char* password)
{
	if (!SendCommandWithResponse("AT\r\n", "OK\r\n"))
	{
		return false;		
	}
	if (!SendCommandWithResponse("AT+GMR\r\n", "OK\r\n"))
	{
		return false;		
	}
	
	// Get version
	char* verStr = strstr((char*)buf, "AT version:");
	char * atVer = strtok(verStr, "\r\n");
	memset(at_version, 0x00, sizeof(at_version));
	strcpy(at_version, atVer + 11);
	verStr = strstr((char*)buf, "SDK version:");
	char* sdkVer = strtok(verStr, "\r\n");
	memset(sdk_version, 0x00, sizeof(sdk_version));
	strcpy(sdk_version, sdkVer + 12);

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

time_t  ESP8266::FindTimeFromServer(const char* url)
{
	if (!IsConnected())
	{
		return 0;
	}
	memset(send_buf, 0x00, sizeof(send_buf));
	sprintf(send_buf, "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", url);
	if (!SendCommandWithResponse((const char*)send_buf, "CONNECT\r\n", 500)) return 0;

	if (!SendCommandWithResponse("AT+CIPSEND=15\r\n", "OK\r\n>")) return 0;
	if (!SendCommandWithResponse("HEAD HTTP/1.1\r\n", "SEND OK\r\n", 500)) return 0;
	char* timeFromResp = strstr((char*)buf, "Date:");
	timeFromResp = strstr(timeFromResp, ", ");
	char * cutedTimeStr = strtok(timeFromResp, "\r\n");
	memset(send_buf, 0x00, sizeof(send_buf));
	strcpy(send_buf, cutedTimeStr + 2);
	//parse date-time
	struct tm tm;
	if (strptime(send_buf, "%d %b %Y %H:%M:%S GMT", &tm) != NULL)
	{
		return mktime(&tm);	
	}
	return 0;
}

int8_t ESP8266::StrMonthToInt(const char* month)
{
	return 7;
}
