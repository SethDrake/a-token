
#pragma once
#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

#ifdef __cplusplus
extern "C" {
#endif
 
extern void SystemInit();

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
//void SVC_Handler(void);
void DebugMon_Handler(void);
//void PendSV_Handler(void);
void SysTick_Handler(void);
void DMA1_Channel6_IRQHandler(void);
void TIM1_UP_IRQHandler(void);
void USART1_IRQHandler(void);

extern void hard_fault_handler(unsigned int * hardfault_args);

#ifdef __cplusplus
}
#endif

#endif /* __INTERRUPTS_H */


