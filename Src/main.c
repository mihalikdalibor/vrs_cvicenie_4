/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);


/* Function processing DMA Rx data. Counts how many capital and small letters are in sentence.
 * Result is supposed to be stored in global variable of type "letter_count_" that is defined in "main.h"
 *
 * @param1 - received sign
 */
void proccesDmaData(uint8_t sign);


/* Space for your global variables. */

	// type your global variables here:
#define STATUS_PERIOD_TICKS	200		/* 200 x 10 ms = 2 s -> 0.5 Hz */
#define MAX_STRING_LEN		35		/* max. chars after '#' (including '$') */

letter_count_ letter_count = {0};		/* result for the last valid string */
char valid_string[MAX_STRING_LEN];		/* last valid string without '#' and '$' */
volatile uint8_t valid_string_ready = 0;	/* set in proccesDmaData (interrupt), cleared in main */
char tx_buffer[128];					/* read by DMA during Tx - must not be local */


int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* Configure the system clock */
  SystemClock_Config();
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();

  /* Space for your local variables, callback registration ...*/

  	  //type your code here:
  USART2_RegisterCallback(proccesDmaData);
  uint16_t status_ticks = 0;
  int len;

  while (1)
  {
	  /* Periodic transmission of information about DMA Rx buffer state.
	   * Transmission frequency - 0.5Hz.
	   * Message format - "Buffer capacity: %d bytes, occupied memory: %d bytes, load [in %]: %f%"
	   * Example message (what I wish to see in terminal) - Buffer capacity: 1000 bytes, occupied memory: 231 bytes, load [in %]: 23.1%
	   */

	  /* Valid text string information transmission.
	   * Transmission frequency - when new valid string is received.
	   * Message format - "Valid string: %s, lower-case: %d, upper-case: %d"
	   * Example message (what I wish to see in terminal) - Valid string: Platn15uborZnakov, lower-case: 13, upper-case: 2
	   */

  	  	  	  //type your code here:
	  LL_mDelay(10);
	  status_ticks++;

	  if(status_ticks >= STATUS_PERIOD_TICKS)
	  {
		  status_ticks = 0;
		  uint16_t occupied = DMA_USART2_BUFFER_SIZE - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_6);
		  uint16_t load_x10 = occupied * 1000 / DMA_USART2_BUFFER_SIZE;	/* load in 0.1 % */

		  while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7));		/* wait for previous Tx */
		  len = snprintf(tx_buffer, sizeof(tx_buffer),
				  "Buffer capacity: %d bytes, occupied memory: %d bytes, load [in %%]: %d.%d%%\r\n",
				  DMA_USART2_BUFFER_SIZE, occupied, load_x10 / 10, load_x10 % 10);
		  USART2_PutBuffer((uint8_t*)tx_buffer, len);
	  }

	  if(valid_string_ready)
	  {
		  valid_string_ready = 0;
		  while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7));		/* wait for previous Tx */
		  len = snprintf(tx_buffer, sizeof(tx_buffer),
				  "Valid string: %s, lower-case: %d, upper-case: %d\r\n",
				  valid_string, letter_count.small_letter, letter_count.capital_letter);
		  USART2_PutBuffer((uint8_t*)tx_buffer, len);
	  }
  }
  /* USER CODE END 3 */
}


void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

  if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0)
  {
  Error_Handler();  
  }
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {
    
  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {
  
  }
  LL_Init1msTick(8000000);
  LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
  LL_SetSystemCoreClock(8000000);
}

/*
 * Implementation of function processing data received via USART.
 */
void proccesDmaData(uint8_t sign)
{
	/* Process received data */

		// type your algorithm here:
	static uint8_t receiving = 0;				/* 1 = start sign '#' was received */
	static char string[MAX_STRING_LEN];			/* chars received after '#' */
	static uint8_t count = 0;					/* number of chars in string */

	if(sign == '#')
	{
		/* start of a new string (also restarts an unfinished one) */
		receiving = 1;
		count = 0;
		return;
	}

	if(!receiving) return;						/* no start sign - ignore */

	if(sign == '$')
	{
		letter_count.small_letter = 0;
		letter_count.capital_letter = 0;
		for(uint8_t i = 0; i < count; i++)
		{
			if(string[i] >= 'a' && string[i] <= 'z') letter_count.small_letter++;
			else if(string[i] >= 'A' && string[i] <= 'Z') letter_count.capital_letter++;
		}

		string[count] = '\0';
		memcpy(valid_string, string, count + 1);
		valid_string_ready = 1;
		receiving = 0;
		return;
	}

	string[count++] = sign;
	if(count >= MAX_STRING_LEN)
	{
		/* 35 chars without '$' - discard and wait for new '#' */
		receiving = 0;
	}
}


void Error_Handler(void)
{

}

#ifdef  USE_FULL_ASSERT

void assert_failed(char *file, uint32_t line)
{ 

}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
