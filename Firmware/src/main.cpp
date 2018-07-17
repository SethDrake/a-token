#include <sys/_stdint.h>

#include <stm32f1xx_hal.h>

#include "cmsis_os.h"

#include "drivers/inc/ssd1306.h"
#include "src/drivers/inc/fonts.h"
#include "periph_config.h"
#include "objects.h"
#include "delay.h"

DMA_HandleTypeDef i2cDmaTx;
UART_HandleTypeDef uart;
I2C_HandleTypeDef i2c;

SSD1306 display;
ESP8266 wifi;

time_t currentTimeGmt;

SYSTEM_MODE systemMode;
WIFI_STATE wifiState;

const char logoStr[] =  " TOTP TOKEN V.0.1";
const char errorStr[] = "    WIFI ERROR!  ";

osThreadId processKeysTaskHandle;
osThreadId drawDataTaskHandle;

void errorHandler(const char* reason);

void DBG_Configuration()
{
	//Enable debug in powersafe modes
	HAL_DBGMCU_EnableDBGSleepMode();
	HAL_DBGMCU_EnableDBGStopMode();
	HAL_DBGMCU_EnableDBGStandbyMode();

	//hard fault on div by zero
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
}

void SystemClock_Configuration()
{
	__IO uint32_t StartUpCounter = 0, HSEStatus;
  
	/* SYSCLK, HCLK, PCLK2 and PCLK1 configuration ---------------------------*/    
	/* Enable HSE */    
	RCC->CR |= ((uint32_t)RCC_CR_HSEON);
 
    /* Wait till HSE is ready and if Time out is reached exit */
	do
	{
		HSEStatus = RCC->CR & RCC_CR_HSERDY;
		StartUpCounter++;  
	} while ((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

	if ((RCC->CR & RCC_CR_HSERDY) != RESET)
	{
		/* Enable Prefetch Buffer */
		FLASH->ACR |= FLASH_ACR_PRFTBE;

		    /* Flash 2 wait state */
		FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
		FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2;    

 
		    /* HCLK = SYSCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
      
		/* PCLK2 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
    
		/* PCLK1 = HCLK */
		RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2;
    
		/*  PLL configuration: PLLCLK = HSE * 9 = 72 MHz */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
		RCC->CFGR |= (uint32_t)(RCC_PLLSOURCE_HSE | RCC_CFGR_PLLMULL9);

		    /* Enable PLL */
		RCC->CR |= RCC_CR_PLLON;

		    /* Wait till PLL is ready */
		while ((RCC->CR & RCC_CR_PLLRDY) == 0)
		{
		}
    
		/* Select PLL as system clock source */
		RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
		RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    

		    /* Wait till PLL is used as system clock source */
		while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08)
		{
		}
	}
	
	SystemCoreClockUpdate();

	/*RCC_PeriphCLKInitTypeDef PeriphClkInit;
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
	PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		errorHandler("PERIPH_CLCK"); //error on init clock
	}*/

	/**Configure the Systick interrupt time */
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);
	/**Configure the Systick */
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
}

void RCC_Configuration()
{

#if (PREFETCH_ENABLE != 0)
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();
#endif
	
	__HAL_RCC_BKP_CLK_ENABLE();
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_I2C1_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();
	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();
}

void I2C_Configuration(I2C_HandleTypeDef* i2cHandle)
{
	i2cHandle->Instance             = I2C1;
	i2cHandle->Init.ClockSpeed      = 400000; //400000
	i2cHandle->Init.DutyCycle       = I2C_DUTYCYCLE_2;
	i2cHandle->Init.OwnAddress1     = 0x00;
	i2cHandle->Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
	i2cHandle->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	i2cHandle->Init.OwnAddress2     = 0x00;
	i2cHandle->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	i2cHandle->Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(i2cHandle) != HAL_OK)
	{
		errorHandler("I2C_INIT");
	}
	__HAL_I2C_ENABLE(i2cHandle);
}

void USART_Configuration(UART_HandleTypeDef* usartHandle)
{
	usartHandle->Instance			= USART1;
	usartHandle->Init.BaudRate		= 115200;
	usartHandle->Init.WordLength	= UART_WORDLENGTH_8B;
	usartHandle->Init.StopBits		= UART_STOPBITS_1;
	usartHandle->Init.Parity		= UART_PARITY_NONE;
	usartHandle->Init.HwFlowCtl		= UART_HWCONTROL_NONE;
	usartHandle->Init.Mode			= UART_MODE_TX_RX;
	usartHandle->Init.OverSampling  = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(usartHandle) != HAL_OK)
	{
		errorHandler("USART_INIT");
	}
}

void DMA_I2C_TX_Configuration(DMA_HandleTypeDef* dmaHandle)
{
	dmaHandle->Instance                 = DMA1_Channel6;
	dmaHandle->Init.Direction           = DMA_MEMORY_TO_PERIPH;
	dmaHandle->Init.PeriphInc           = DMA_PINC_DISABLE;
	dmaHandle->Init.MemInc              = DMA_MINC_ENABLE;
	dmaHandle->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	dmaHandle->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
	dmaHandle->Init.Mode                = DMA_NORMAL;
	dmaHandle->Init.Priority            = DMA_PRIORITY_VERY_HIGH;
	if (HAL_DMA_Init(dmaHandle) != HAL_OK)
	{
		errorHandler("I2C_DMA_INIT");
	}
}

void GPIO_Configuration(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
		/* Configure I2C pins: SCL and SDA */
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pin = I2C_SCL | I2C_SDA;
	HAL_GPIO_Init(I2C_PORT, &GPIO_InitStruct);

		/* USART1 PINS */
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pin = USART_TX;
	HAL_GPIO_Init(USART_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pin = USART_RX;
	HAL_GPIO_Init(USART_PORT, &GPIO_InitStruct);

		/* Configure KEYBOARD pins */
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Pin = KBRD_PIN;
	HAL_GPIO_Init(KBRD_PORT, &GPIO_InitStruct);

		/* Configure LED pins */
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Pin = LED_A | LED_B;
	HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

	__HAL_AFIO_REMAP_SWJ_NOJTAG(); //disable JTAG
}

void EXTI_Configuration()
{

}

void NVIC_Configuration(void) {
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* System interrupt init*/
  /* MemoryManagement_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
	/* BusFault_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
	/* UsageFault_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
	/* SVCall_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SVCall_IRQn, 0, 0);
	/* DebugMonitor_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);
	/* PendSV_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 14, 0);

	HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 15, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

	HAL_NVIC_SetPriority(USART1_IRQn, 15, 0);
	HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void switchSystemMode(SYSTEM_MODE mode)
{
	if (mode == systemMode)
	{
		return;	
	}
	if (mode == SLEEP)
	{
		systemMode = SLEEP;
	}
	else
	{
		systemMode = ACTIVE;
	}
}


void drawDataTask(void const * argument)
{
	uint16_t cntr = 0;
	bool point = true;
	while (true)
	{
		if (systemMode == ACTIVE && display.IsSleep())
		{
			display.sleepMode(false);
		}
		else if (systemMode == SLEEP && !display.IsSleep())
		{
			display.sleepMode(true);
		}
		if (!display.IsSleep())
		{
			display.clearScreen();
			display.printf(12, 50, logoStr);
			display.drawLine(0, 44, 127, 44);
			if (wifiState == WIFI_LOADING)
			{
				display.printf(12, 30, "  WIFI LOADING   ");	
			}
			else if (wifiState == WIFI_CONNECTED)
			{
				display.printf(12, 30, "  WIFI CONNECTED ");
				currentTimeGmt = wifi.FindTimeFromServer("www.google.com");
				char* timestr = ctime(&currentTimeGmt);
				display.printf(10, 5, timestr);
			}
			else if (wifiState == WIFI_ERROR)
			{
				display.printf(12, 30, "  WIFI ERROR     ");	
			}
			display.drawLine(0, 62, 1, 62, point);
			display.drawLine(0, 63, 1, 63, point);
			point = !point;
			display.drawFramebuffer();
		}

		osDelay(1000);
	}
}


void processKeysTask(void const * argument)
{
	uint16_t slpCntr = 0;
	bool prevState = false;
	while (true)
	{
		HAL_GPIO_TogglePin(LED_PORT, LED_B);
		if (systemMode == LOADING)
		{
			osDelay(350);
			return;
		}
		bool keyPressed = HAL_GPIO_ReadPin(KBRD_PORT, KBRD_PIN);
		if (keyPressed)
		{
			if (systemMode == ACTIVE)
			{
				if (prevState) //long press
				{
					switchSystemMode(SLEEP);
					osDelay(500);
				}
				else //short press
				{
					//
				}
			}
			else
			{
				switchSystemMode(ACTIVE);
				osDelay(500);
			}
			slpCntr = 0;
		}
		prevState = keyPressed;
		if (systemMode == ACTIVE)
		{
			slpCntr++;
		}
		if (slpCntr / 3 >= 120)
		{
			//switchSystemMode(SLEEP);
			slpCntr = 0;
		}
		osDelay(350);
	}
}


int main()
{
	DBG_Configuration();
	SystemClock_Configuration();
	DelayManager::DelayMs(150);
	RCC_Configuration();
	GPIO_Configuration();
	EXTI_Configuration();
	NVIC_Configuration();
	I2C_Configuration(&i2c);
	USART_Configuration(&uart);
	DMA_I2C_TX_Configuration(&i2cDmaTx);
	//__HAL_LINKDMA(&i2c, hdmatx, i2cDmaTx);

	wifiState = WIFI_LOADING;
	systemMode = LOADING;

	display.initDisplay(&i2c);
	display.setFont(font5x7);
	display.clearScreen();
	display.printf(12, 50, logoStr);
	display.drawLine(0, 44, 127, 44);
	display.printf(12, 15, ".... LOADING ....");
	display.drawFramebuffer();

	wifi.initModule(&uart, &display);
	wifiState = WIFI_LOADING;
	display.printf(12, 15, ".. WIFI CONFIG ..");
	display.drawFramebuffer();
	if (wifi.ConfigureModule("GAMMA_14N", "Access117"))
	{
		wifiState = WIFI_CONNECTED;
	}
	else
	{
		wifiState = WIFI_ERROR;		
	}

	systemMode = ACTIVE;

	osThreadDef(processKeysThread, processKeysTask, osPriorityLow, 0, configMINIMAL_STACK_SIZE);
	processKeysTaskHandle = osThreadCreate(osThread(processKeysThread), NULL);

	osThreadDef(drawDataThread, drawDataTask, osPriorityHigh, 0, 256);
	drawDataTaskHandle = osThreadCreate(osThread(drawDataThread), NULL);

	osKernelStart();
	
	while (true)
	{
	}
}

void errorHandler(const char* reason)
{
	if (display.IsActive() && reason != NULL)
	{
		display.printf(0, 56, "SYSTEM ERROR!");
		display.printf(0, 40, "%s", *reason);
	}
	
	while (true)
	{
		HAL_GPIO_TogglePin(LED_PORT, LED_A);
		DelayManager::DelayMs(500);
	}
}
