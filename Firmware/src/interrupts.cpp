
#include <stm32f1xx_hal.h>
#include <cmsis_os.h>
#include "interrupts.h"
#include "delay.h"
#include "objects.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief   This function handles NMI exception.
* @param  None
* @retval None
*/
	void __attribute__((interrupt("IRQ"))) NMI_Handler(void)
{
}

void __attribute__((interrupt("IRQ"))) HardFault_Handler(void)
{
	__asm__("TST LR, #4");
	__asm__("ITE EQ");
	__asm__("MRSEQ R0, MSP");
	__asm__("MRSNE R0, PSP");
	__asm__("B hard_fault_handler");
}

/**
* @brief  This function handles Memory Manage exception.
* @param  None
* @retval None
*/
void __attribute__((interrupt("IRQ"))) MemManage_Handler(void)
{
	/* Go to infinite loop when Memory Manage exception occurs */
	while (1)
	{
	}
}

/**
* @brief  This function handles Bus Fault exception.
* @param  None
* @retval None
*/void __attribute__((interrupt("IRQ"))) BusFault_Handler(void)
{
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1)
	{
	}
}

/**
* @brief  This function handles Usage Fault exception.
* @param  None
* @retval None
*/
void __attribute__((interrupt("IRQ"))) UsageFault_Handler(void)
{
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1)
	{
	}
}

/*void __attribute__((interrupt("IRQ"))) SVC_Handler(void)
{
}*/

void __attribute__((interrupt("IRQ"))) DebugMon_Handler(void)
{
}

/*void __attribute__((interrupt("IRQ"))) PendSV_Handler(void)
{
}*/

void __attribute__((interrupt("IRQ"))) SysTick_Handler(void) {
	//DelayManager::SysTickIncrement();
	osSystickHandler();
	HAL_IncTick();
}

void __attribute__((interrupt("IRQ"))) DMA1_Channel6_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&i2cDmaTx);
}


void __attribute__((interrupt("IRQ"))) TIM1_UP_IRQHandler(void)
{
	//HAL_TIM_IRQHandler(&htim1);	
}
	
void __attribute__((interrupt("IRQ"))) USART1_IRQHandler(void)
{
	wifi.OnRxInterrupt();	
}

void hard_fault_handler(unsigned int * hardfault_args)
{
	unsigned int stacked_r0;
	unsigned int stacked_r1;
	unsigned int stacked_r2;
	unsigned int stacked_r3;
	unsigned int stacked_r12;
	unsigned int stacked_lr;
	unsigned int stacked_pc;
	unsigned int stacked_psr;

	stacked_r0 = ((unsigned long) hardfault_args[0]);
	stacked_r1 = ((unsigned long) hardfault_args[1]);
	stacked_r2 = ((unsigned long) hardfault_args[2]);
	stacked_r3 = ((unsigned long) hardfault_args[3]);

	stacked_r12 = ((unsigned long) hardfault_args[4]);
	stacked_lr = ((unsigned long) hardfault_args[5]);
	stacked_pc = ((unsigned long) hardfault_args[6]);
	stacked_psr = ((unsigned long) hardfault_args[7]);

	if (display.IsActive())
	{
		display.printf(10, 56, "HARD FAULT!");
		display.printf(10, 40, "LR = %x", stacked_lr);
		display.printf(10, 30, "PC = %x", stacked_pc);
		display.printf(10, 20, "PSR = %x", stacked_psr);
	}


	while (1);
}

#ifdef __cplusplus
}
#endif
