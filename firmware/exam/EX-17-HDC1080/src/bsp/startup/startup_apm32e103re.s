/**
  *************** (C) COPYRIGHT 2017 STMicroelectronics ************************
  * @file      startup_stm32f103xe.s
  * @author    MCD Application Team
  * @brief     STM32F103xE Devices vector table for Atollic toolchain.
  *            This module performs:
  *                - Set the initial SP
  *                - Set the initial PC == Reset_Handler,
  *                - Set the vector table entries with the exceptions ISR address
  *                - Configure the clock system   
  *                - Configure external SRAM mounted on STM3210E-EVAL board
  *                  to be used as data memory (optional, to be enabled by user)
  *                - Branches to main in the C library (which eventually
  *                  calls main()).
  *            After Reset the Cortex-M3 processor is in Thread mode,
  *            priority is Privileged, and the Stack is set to Main.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

  .syntax unified
  .cpu cortex-m3
  .fpu softvfp
  .thumb

.global g_pfnVectors
.global Default_Handler

/* start address for the initialization values of the .data section.
defined in linker script */
.word _sidata
/* start address for the .data section. defined in linker script */
.word _sdata
/* end address for the .data section. defined in linker script */
.word _edata
/* start address for the .bss section. defined in linker script */
.word _sbss
/* end address for the .bss section. defined in linker script */
.word _ebss

/**
 * @brief  This is the code that gets called when the processor first
 *          starts execution following a reset event. Only the absolutely
 *          necessary set is performed, after which the application
 *          supplied main() routine is called.
 * @param  None
 * @retval : None
*/

  .section .text.Reset_Handler
  .weak Reset_Handler
  .type Reset_Handler, %function
Reset_Handler:
  ldr   sp, =_estack     /* set stack pointer */

/* Call the clock system initialization function.*/
  bl  SystemInit

/* Copy the data segment initializers from flash to SRAM */
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
  movs r3, #0
  b LoopCopyDataInit

CopyDataInit:
  ldr r4, [r2, r3]
  str r4, [r0, r3]
  adds r3, r3, #4

LoopCopyDataInit:
  adds r4, r0, r3
  cmp r4, r1
  bcc CopyDataInit
  
/* Zero fill the bss segment. */
  ldr r2, =_sbss
  ldr r4, =_ebss
  movs r3, #0
  b LoopFillZerobss

FillZerobss:
  str  r3, [r2]
  adds r2, r2, #4

LoopFillZerobss:
  cmp r2, r4
  bcc FillZerobss

/* Call static constructors */
    bl __libc_init_array
/* Call the application's entry point.*/
  bl main
  bx lr
.size Reset_Handler, .-Reset_Handler

/**
 * @brief  This is the code that gets called when the processor receives an
 *         unexpected interrupt.  This simply enters an infinite loop, preserving
 *         the system state for examination by a debugger.
 *
 * @param  None
 * @retval : None
*/
    .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b Infinite_Loop
  .size Default_Handler, .-Default_Handler
/******************************************************************************
*
* The minimal vector table for a Cortex M3.  Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
*
******************************************************************************/
  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object
  .size g_pfnVectors, .-g_pfnVectors


g_pfnVectors:

  .word _estack
  .word Reset_Handler
  .word NMI_Handler
  .word HardFault_Handler
  .word MemManage_Handler
  .word BusFault_Handler
  .word UsageFault_Handler
  .word 0
  .word 0
  .word 0
  .word 0
  .word SVC_Handler
  .word DebugMon_Handler
  .word 0
  .word PendSV_Handler
  .word SysTick_Handler
  .word WWDG_IRQHandler
  .word PVD_IRQHandler
  .word TAMPER_IRQHandler
  .word RTC_IRQHandler                      // RTC
  .word FLASH_IRQHandler                    // Flash
  .word RCM_IRQHandler                      // RCM
  .word EINT0_IRQHandler                    // EINT Line 0
  .word EINT1_IRQHandler                    // EINT Line 1
  .word EINT2_IRQHandler                    // EINT Line 2
  .word EINT3_IRQHandler                    // EINT Line 3
  .word EINT4_IRQHandler                    // EINT Line 4
  .word DMA1_Channel1_IRQHandler            // DMA1 Channel 1
  .word DMA1_Channel2_IRQHandler            // DMA1 Channel 2
  .word DMA1_Channel3_IRQHandler            // DMA1 Channel 3
  .word DMA1_Channel4_IRQHandler            // DMA1 Channel 4
  .word DMA1_Channel5_IRQHandler            // DMA1 Channel 5
  .word DMA1_Channel6_IRQHandler            // DMA1 Channel 6
  .word DMA1_Channel7_IRQHandler            // DMA1 Channel 7
  .word ADC1_2_IRQHandler                   // ADC1 & ADC2
  .word USBD1_HP_CAN1_TX_IRQHandler         // USBD1 High Priority or CAN1 TX
  .word USBD1_LP_CAN1_RX0_IRQHandler        // USBD1 Low  Priority or CAN1 RX0
  .word CAN1_RX1_IRQHandler                 // CAN1 RX1
  .word CAN1_SCE_IRQHandler                 // CAN1 SCE
  .word EINT9_5_IRQHandler                  // EINT Line 9..5
  .word TMR1_BRK_IRQHandler                 // TMR1 Break
  .word TMR1_UP_IRQHandler                  // TMR1 Update
  .word TMR1_TRG_COM_IRQHandler             // TMR1 Trigger and Commutation
  .word TMR1_CC_IRQHandler                  // TMR1 Capture Compare
  .word TMR2_IRQHandler                     // TMR2
  .word TMR3_IRQHandler                     // TMR3
  .word TMR4_IRQHandler                     // TMR4
  .word I2C1_EV_IRQHandler                  // I2C1 Event
  .word I2C1_ER_IRQHandler                  // I2C1 Error
  .word I2C2_EV_IRQHandler                  // I2C2 Event
  .word I2C2_ER_IRQHandler                  // I2C2 Error
  .word SPI1_IRQHandler                     // SPI1
  .word SPI2_IRQHandler                     // SPI2
  .word USART1_IRQHandler                   // USART1
  .word USART2_IRQHandler                   // USART2
  .word USART3_IRQHandler                   // USART3
  .word EINT15_10_IRQHandler                // EINT Line 15..10
  .word RTCAlarm_IRQHandler                 // RTC Alarm through EINT Line
  .word USBDWakeUp_IRQHandler               // USBD Wakeup from suspend
  .word TMR8_BRK_IRQHandler                 // TMR8 Break
  .word TMR8_UP_IRQHandler                  // TMR8 Update
  .word TMR8_TRG_COM_IRQHandler             // TMR8 Trigger and Commutation
  .word TMR8_CC_IRQHandler                  // TMR8 Capture Compare
  .word ADC3_IRQHandler                     // ADC3
  .word EMMC_IRQHandler                     // EMMC
  .word SDIO_IRQHandler                     // SDIO
  .word TMR5_IRQHandler                     // TMR5
  .word SPI3_IRQHandler                     // SPI3
  .word UART4_IRQHandler                    // UART4
  .word UART5_IRQHandler                    // UART5
  .word TMR6_IRQHandler                     // TMR6
  .word TMR7_IRQHandler                     // TMR7
  .word DMA2_Channel1_IRQHandler            // DMA2 Channel1
  .word DMA2_Channel2_IRQHandler            // DMA2 Channel2
  .word DMA2_Channel3_IRQHandler            // DMA2 Channel3
  .word DMA2_Channel4_5_IRQHandler          // DMA2 Channel4 & Channel5
  .word 0                                   // Reserved
  .word USBD2_HP_CAN2_TX_IRQHandler         // USBD2 High Priority or CAN2 TX
  .word USBD2_LP_CAN2_RX0_IRQHandler        // USBD2 Low  Priority or CAN2 RX0
  .word CAN2_RX1_IRQHandler                 // CAN2 RX1
  .word CAN2_SCE_IRQHandler                 // CAN2 SCE



/*******************************************************************************
*
* Provide weak aliases for each Exception handler to the Default_Handler.
* As they are weak aliases, any function with the same name will override
* this definition.
*
*******************************************************************************/

  .weak NMI_Handler
  .thumb_set NMI_Handler,Default_Handler

  .weak HardFault_Handler
  .thumb_set HardFault_Handler,Default_Handler

  .weak MemManage_Handler
  .thumb_set MemManage_Handler,Default_Handler

  .weak BusFault_Handler
  .thumb_set BusFault_Handler,Default_Handler

  .weak UsageFault_Handler
  .thumb_set UsageFault_Handler,Default_Handler

  .weak SVC_Handler
  .thumb_set SVC_Handler,Default_Handler

  .weak DebugMon_Handler
  .thumb_set DebugMon_Handler,Default_Handler

  .weak PendSV_Handler
  .thumb_set PendSV_Handler,Default_Handler

  .weak SysTick_Handler
  .thumb_set SysTick_Handler,Default_Handler

  .weak WWDG_IRQHandler
  .thumb_set WWDG_IRQHandler,Default_Handler

  .weak PVD_IRQHandler
  .thumb_set PVD_IRQHandler,Default_Handler

  .weak TAMPER_IRQHandler
  .thumb_set TAMPER_IRQHandler,Default_Handler

  .weak RTC_IRQHandler
  .thumb_set RTC_IRQHandler,Default_Handler

  .weak FLASH_IRQHandler
  .thumb_set FLASH_IRQHandler,Default_Handler

  .weak RCM_IRQHandler
  .thumb_set RCM_IRQHandler,Default_Handler

  .weak EINT0_IRQHandler
  .thumb_set EINT0_IRQHandler,Default_Handler

  .weak EINT1_IRQHandler
  .thumb_set EINT1_IRQHandler,Default_Handler

  .weak EINT2_IRQHandler
  .thumb_set EINT2_IRQHandler,Default_Handler

  .weak EINT3_IRQHandler
  .thumb_set EINT3_IRQHandler,Default_Handler

  .weak EINT4_IRQHandler
  .thumb_set EINT4_IRQHandler,Default_Handler

  .weak DMA1_Channel1_IRQHandler
  .thumb_set DMA1_Channel1_IRQHandler,Default_Handler

  .weak DMA1_Channel2_IRQHandler
  .thumb_set DMA1_Channel2_IRQHandler,Default_Handler

  .weak DMA1_Channel3_IRQHandler
  .thumb_set DMA1_Channel3_IRQHandler,Default_Handler

  .weak DMA1_Channel4_IRQHandler
  .thumb_set DMA1_Channel4_IRQHandler,Default_Handler

  .weak DMA1_Channel5_IRQHandler
  .thumb_set DMA1_Channel5_IRQHandler,Default_Handler

  .weak DMA1_Channel6_IRQHandler
  .thumb_set DMA1_Channel6_IRQHandler,Default_Handler

  .weak DMA1_Channel7_IRQHandler
  .thumb_set DMA1_Channel7_IRQHandler,Default_Handler

  .weak ADC1_2_IRQHandler
  .thumb_set ADC1_2_IRQHandler,Default_Handler

  .weak USBD1_HP_CAN1_TX_IRQHandler
  .thumb_set USBD1_HP_CAN1_TX_IRQHandler,Default_Handler

  .weak USBD1_LP_CAN1_RX0_IRQHandler
  .thumb_set USBD1_LP_CAN1_RX0_IRQHandler,Default_Handler

  .weak CAN1_RX1_IRQHandler
  .thumb_set CAN1_RX1_IRQHandler,Default_Handler

  .weak CAN1_SCE_IRQHandler
  .thumb_set CAN1_SCE_IRQHandler,Default_Handler

  .weak EINT9_5_IRQHandler
  .thumb_set EINT9_5_IRQHandler,Default_Handler

  .weak TMR1_BRK_IRQHandler
  .thumb_set TMR1_BRK_IRQHandler,Default_Handler

  .weak TMR1_UP_IRQHandler
  .thumb_set TMR1_UP_IRQHandler,Default_Handler

  .weak TMR1_TRG_COM_IRQHandler
  .thumb_set TMR1_TRG_COM_IRQHandler,Default_Handler

  .weak TMR1_CC_IRQHandler
  .thumb_set TMR1_CC_IRQHandler,Default_Handler

  .weak TMR2_IRQHandler
  .thumb_set TMR2_IRQHandler,Default_Handler

  .weak TMR3_IRQHandler
  .thumb_set TMR3_IRQHandler,Default_Handler

  .weak TMR4_IRQHandler
  .thumb_set TMR4_IRQHandler,Default_Handler

  .weak I2C1_EV_IRQHandler
  .thumb_set I2C1_EV_IRQHandler,Default_Handler

  .weak I2C1_ER_IRQHandler
  .thumb_set I2C1_ER_IRQHandler,Default_Handler

  .weak I2C2_EV_IRQHandler
  .thumb_set I2C2_EV_IRQHandler,Default_Handler

  .weak I2C2_ER_IRQHandler
  .thumb_set I2C2_ER_IRQHandler,Default_Handler

  .weak SPI1_IRQHandler
  .thumb_set SPI1_IRQHandler,Default_Handler

  .weak SPI2_IRQHandler
  .thumb_set SPI2_IRQHandler,Default_Handler

  .weak USART1_IRQHandler
  .thumb_set USART1_IRQHandler,Default_Handler

  .weak USART2_IRQHandler
  .thumb_set USART2_IRQHandler,Default_Handler

  .weak USART3_IRQHandler
  .thumb_set USART3_IRQHandler,Default_Handler

  .weak EINT15_10_IRQHandler
  .thumb_set EINT15_10_IRQHandler,Default_Handler

  .weak RTCAlarm_IRQHandler
  .thumb_set RTCAlarm_IRQHandler,Default_Handler

  .weak USBDWakeUp_IRQHandler
  .thumb_set USBDWakeUp_IRQHandler,Default_Handler

  .weak TMR8_BRK_IRQHandler
  .thumb_set TMR8_BRK_IRQHandler,Default_Handler

  .weak TMR8_UP_IRQHandler
  .thumb_set TMR8_UP_IRQHandler,Default_Handler

  .weak TMR8_TRG_COM_IRQHandler
  .thumb_set TMR8_TRG_COM_IRQHandler,Default_Handler

  .weak TMR8_CC_IRQHandler
  .thumb_set TMR8_CC_IRQHandler,Default_Handler

  .weak ADC3_IRQHandler
  .thumb_set ADC3_IRQHandler,Default_Handler

  .weak EMMC_IRQHandler
  .thumb_set EMMC_IRQHandler,Default_Handler

  .weak SDIO_IRQHandler
  .thumb_set SDIO_IRQHandler,Default_Handler

  .weak TMR5_IRQHandler
  .thumb_set TMR5_IRQHandler,Default_Handler

  .weak SPI3_IRQHandler
  .thumb_set SPI3_IRQHandler,Default_Handler

  .weak UART4_IRQHandler
  .thumb_set UART4_IRQHandler,Default_Handler

  .weak UART5_IRQHandler
  .thumb_set UART5_IRQHandler,Default_Handler

  .weak TMR6_IRQHandler
  .thumb_set TMR6_IRQHandler,Default_Handler

  .weak TMR7_IRQHandler
  .thumb_set TMR7_IRQHandler,Default_Handler

  .weak DMA2_Channel1_IRQHandler
  .thumb_set DMA2_Channel1_IRQHandler,Default_Handler

  .weak DMA2_Channel2_IRQHandler
  .thumb_set DMA2_Channel2_IRQHandler,Default_Handler

  .weak DMA2_Channel3_IRQHandler
  .thumb_set DMA2_Channel3_IRQHandler,Default_Handler

  .weak DMA2_Channel4_5_IRQHandler
  .thumb_set DMA2_Channel4_5_IRQHandler,Default_Handler

  .weak USBD2_HP_CAN2_TX_IRQHandler
  .thumb_set USBD2_HP_CAN2_TX_IRQHandler,Default_Handler

  .weak USBD2_LP_CAN2_RX0_IRQHandler
  .thumb_set USBD2_LP_CAN2_RX0_IRQHandler,Default_Handler

  .weak CAN2_RX1_IRQHandler
  .thumb_set CAN2_RX1_IRQHandler,Default_Handler

  .weak CAN2_SCE_IRQHandler
  .thumb_set CAN2_SCE_IRQHandler,Default_Handler

