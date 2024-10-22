/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include "lis2de12_reg.h"
#include "RGBLed.h"
#include "Accelerometer.h"
#include "UserTimer.h"
#include "SerialCommands.h"
#include "Settings.h"
#include "fw_version.h"
#include <string.h>
#include <stdio.h>
#include "MinMaxTracker.h"
#include "Utilities.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TACKLE_THRESHOLD 6000.0f // mg
const float accel_filter_alpha = 0.80f;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
IWDG_HandleTypeDef hiwdg;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim16;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart2_rx;

/* USER CODE BEGIN PV */
UserTimer timer_ctx = {0};
SerialCommands command_ctx = {0};
uint8_t rx_buff[50] = {0};
void uart_dma_transfer_complete(DMA_HandleTypeDef *_hdma);
MinMaxTracker accel_x_min_max = {0};
MinMaxTracker accel_y_min_max = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_IWDG_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM16_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


void SetRGBValue(const char* msg, uint32_t msg_len);
void SetFadeEnable(const char* msg, uint32_t msg_len);
void GetAcceleration(const char* msg, uint32_t msg_len);
void GetAccelMagRange(const char* msg, uint32_t msglen);
void GetHomeAwayStatus(const char* msg, uint32_t msg_len);
void GetEligibleStatus(const char* msg, uint32_t msg_len);
void GetTackledStatus(const char* msg, uint32_t msg_len);
void GetFirmwareVersion(const char* msg, uint32_t msg_len);

#define NUM_COMMANDS	8
const Command commands[NUM_COMMANDS] = {
    {
        .command_str = "l",
        .command_func = SetRGBValue
    },
	{
		.command_str = "f",
		.command_func = SetFadeEnable
	},
	{
		.command_str = "a",
		.command_func = GetAcceleration
	},
	{
		.command_str = "r",
		.command_func = GetAccelMagRange
	},
	{
		.command_str = "h",
		.command_func = GetHomeAwayStatus
	},
	{
		.command_str = "e",
		.command_func = GetEligibleStatus
	},
	{
		.command_str = "t",
		.command_func = GetTackledStatus
	},
	{
		.command_str = "v",
		.command_func = GetFirmwareVersion
	},
};
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  MinMaxTracker_Reset(&accel_x_min_max);
  MinMaxTracker_Reset(&accel_y_min_max);

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_IWDG_Init();
  MX_TIM1_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */
  printf("Robotic Football Tackle Sensor\n");
  printf("Version: %d.%d.%d\n", FW_MAJOR_VER, FW_MINOR_VER, FW_PATCH_VER);
  Settings_Init();
  RGBLed_Init();
  if( !Accelerometer_Init() )
  {
	  printf("Failed to initialize accelerometer.\n");
	  HAL_Delay(500);
	  if( !Accelerometer_Init() )
	  {
		  printf("Failed to initialize accelerometer.\n");
		  HAL_Delay(500);
		  if( !Accelerometer_Init() )
		  {
			  printf("Failed to initialize accelerometer.\n");

			  // System Failed to start properly.  This indicates serious damage to the sensor and the sensor won't work.
			  // Let the user know.
			  HAL_GPIO_WritePin(TACKLE_STATUS_GPIO_Port, TACKLE_STATUS_Pin, GPIO_PIN_RESET);
			  while(1)
			  {
				  for( int i = 0; i < 3; i++ )
				  {
					  RGBLed_SetRed(true);
					  HAL_Delay(200);
					  RGBLed_SetOff();
					  HAL_Delay(200);
				  }
				  for( int i = 0; i < 3; i++ )
				  {
					  RGBLed_SetRed(true);
					  HAL_Delay(400);
					  RGBLed_SetOff();
					  HAL_Delay(400);
				  }
				  for( int i = 0; i < 3; i++ )
				  {
					  RGBLed_SetRed(true);
					  HAL_Delay(200);
					  RGBLed_SetOff();
					  HAL_Delay(200);
				  }
				  HAL_Delay(500);
			  }
		  }
	  }
  }

  UserTimer_Init(&timer_ctx, 2000);
  SerialCommands_Init(&command_ctx, commands, NUM_COMMANDS, "\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  __HAL_IWDG_START(&hiwdg);
  Accelerometer_Data data = {0};
  float smoothed_data_x = 0;
  float smoothed_data_y = 0;
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  HAL_IWDG_Refresh(&hiwdg);
	  if( Accelerometer_Update() )
	  {
		  // New Data

			Accelerometer_GetData( &data );

			// Run raw readings through IIRFilter
			smoothed_data_x = IIRFilter( data.x/8000.0f, smoothed_data_x/8000.0f, accel_filter_alpha)*8000.0f;
			smoothed_data_y = IIRFilter( data.y/8000.0f, smoothed_data_y/8000.0f, accel_filter_alpha)*8000.0f;

			// Update MinMaxTracker
			MinMaxTracker_Update(&accel_x_min_max, smoothed_data_x);
			MinMaxTracker_Update(&accel_y_min_max, smoothed_data_y);

			if( fabsf(smoothed_data_x) > TACKLE_THRESHOLD
			  || fabsf(smoothed_data_y) > TACKLE_THRESHOLD )
			{
			  UserTimer_Start(&timer_ctx);
			}
	  }

	  bool is_tackled = UserTimer_GetActive(&timer_ctx);

	  if(HAL_GPIO_ReadPin(ELIGIBLE_SELECT_GPIO_Port, ELIGIBLE_SELECT_Pin) == GPIO_PIN_SET)
	  {
		  // In-eligible Receiver
		  if( Settings_GetFade() )
		  {
			  // Fade Mode
			  RGBLed_EnablePulse();
			  if(HAL_GPIO_ReadPin(HOME_SELECT_GPIO_Port, HOME_SELECT_Pin) == GPIO_PIN_RESET)
			  {
				  // Home
				  uint8_t r = Settings_GetHomeRed();
				  uint8_t g = Settings_GetHomeGreen();
				  uint8_t b = Settings_GetHomeBlue();
				  RGBLed_SetManual(r, g, b, false);
			  }
			  else
			  {
				  // Away
				  RGBLed_SetWhite(false);
			  }
		  }
		  else
		  {
			  // Off Mode
			  RGBLed_DisablePulse();
			  RGBLed_SetOff();
		  }
		  HAL_GPIO_WritePin(TACKLE_STATUS_GPIO_Port, TACKLE_STATUS_Pin, GPIO_PIN_SET);
	  }
	  else
	  {
		  // Eligible Receiver
		  RGBLed_DisablePulse();
		  if( is_tackled )
		  {
			  RGBLed_SetRed(true);
			  HAL_GPIO_WritePin(TACKLE_STATUS_GPIO_Port, TACKLE_STATUS_Pin, GPIO_PIN_RESET);
		  }
		  else
		  {
			  if(HAL_GPIO_ReadPin(HOME_SELECT_GPIO_Port, HOME_SELECT_Pin) == GPIO_PIN_RESET)
			  {
				  // Home
				  uint8_t r = Settings_GetHomeRed();
				  uint8_t g = Settings_GetHomeGreen();
				  uint8_t b = Settings_GetHomeBlue();
				  RGBLed_SetManual(r, g, b, true);
			  }
			  else
			  {
				  // Away
				  RGBLed_SetWhite(true);
			  }
			  HAL_GPIO_WritePin(TACKLE_STATUS_GPIO_Port, TACKLE_STATUS_Pin, GPIO_PIN_SET);
		  }
	  }
	  // Process serial commands from main context
	  SerialCommands_Process(&command_ctx);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage 
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the peripherals clocks 
  */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_TIM1;
  PeriphClkInit.Tim1ClockSelection = RCC_TIM1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
  hiwdg.Init.Window = 4095;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 640;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  /* USER CODE BEGIN TIM16_Init 0 */

  /* USER CODE END TIM16_Init 0 */

  /* USER CODE BEGIN TIM16_Init 1 */

  /* USER CODE END TIM16_Init 1 */
  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 64000;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 50;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM16_Init 2 */
  HAL_TIM_Base_Start_IT(&htim16);

  /* USER CODE END TIM16_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  HAL_DMA_RegisterCallback(&hdma_usart2_rx, HAL_DMA_XFER_CPLT_CB_ID, uart_dma_transfer_complete);

  __HAL_UART_CLEAR_IT(&huart2, UART_CLEAR_IDLEF);
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);

  HAL_UART_Receive_DMA(&huart2, rx_buff, sizeof(rx_buff));
  /* USER CODE END USART2_Init 2 */

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(TACKLE_STATUS_GPIO_Port, TACKLE_STATUS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SPI1_NSS_Pin|SPI_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : HOME_SELECT_Pin */
  GPIO_InitStruct.Pin = HOME_SELECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(HOME_SELECT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TACKLE_STATUS_Pin */
  GPIO_InitStruct.Pin = TACKLE_STATUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TACKLE_STATUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ELIGIBLE_SELECT_Pin */
  GPIO_InitStruct.Pin = ELIGIBLE_SELECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ELIGIBLE_SELECT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ACC_INT1_Pin ACC_INT2_Pin */
  GPIO_InitStruct.Pin = ACC_INT1_Pin|ACC_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI1_NSS_Pin SPI_RESET_Pin */
  GPIO_InitStruct.Pin = SPI1_NSS_Pin|SPI_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
int __io_putchar(int ch)
{
	HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
	return ch;
}

// Parses Message received in the format <red>,<green>,<blue>
void SetRGBValue(const char* msg, uint32_t msg_len)
{
	int r, g, b = 0;
	int count = sscanf(msg, ":%d,%d,%d\n", &r, &g, &b );
	if( count == 3 )
	{
		r = Clamp( r, 0, 255 );
		g = Clamp( g, 0, 255 );
		b = Clamp( b, 0, 255 );
		Settings_SetHomeRedGreenBlue(r,g,b);
	}
	printf( "l:%d,%d,%d\n", Settings_GetHomeRed(), Settings_GetHomeGreen(), Settings_GetHomeBlue() );
}

// Parses Message received in the format <fade_enable>
void SetFadeEnable(const char* msg, uint32_t msg_len)
{
	int fade = 0;
	int count = sscanf(msg, ":%d\n", &fade );
	if( count == 1 )
	{
		fade = Clamp( fade, 0, 1 );
		Settings_SetFade( fade );
	}
	printf( "f:%d\n", Settings_GetFade() );
}

void GetAcceleration(const char* msg, uint32_t msg_len)
{
	Accelerometer_Data data = {0};
	Accelerometer_GetData( &data );
	printf( "a:%d,%d,%d\n", (int)data.x, (int)data.y, (int)data.z );
}

void GetAccelMagRange(const char* msg, uint32_t msg_len)
{
	// Grab the min and max since the last update
	int xmin = (int)MinMaxTracker_getMin(&accel_x_min_max);
	int xmax = (int)MinMaxTracker_getMax(&accel_x_min_max);
	int ymin = (int)MinMaxTracker_getMin(&accel_y_min_max);
	int ymax = (int)MinMaxTracker_getMax(&accel_y_min_max);
	// Reset
	MinMaxTracker_Reset(&accel_x_min_max);
	MinMaxTracker_Reset(&accel_y_min_max);
	printf( "r:%d,%d,%d,%d\n", xmin, xmax, ymin, ymax);
}

void GetHomeAwayStatus(const char* msg, uint32_t msg_len)
{
	bool home = (HAL_GPIO_ReadPin(HOME_SELECT_GPIO_Port, HOME_SELECT_Pin) == GPIO_PIN_RESET);
	printf("h:%s\n", home ? "1" : "0");
}

void GetEligibleStatus(const char* msg, uint32_t msg_len)
{
	bool eligible = (HAL_GPIO_ReadPin(ELIGIBLE_SELECT_GPIO_Port, ELIGIBLE_SELECT_Pin) == GPIO_PIN_RESET);
	printf("e:%s\n", eligible ? "1" : "0");
}

void GetTackledStatus(const char* msg, uint32_t msg_len)
{
	bool is_tackled = UserTimer_GetActive(&timer_ctx);
	printf("t:%s\n", is_tackled ? "1" : "0");
}

void GetFirmwareVersion(const char* msg, uint32_t msg_len)
{
	printf("v:%d.%d.%d\n", FW_MAJOR_VER, FW_MINOR_VER, FW_PATCH_VER );
}

void uart_dma_transfer_complete(DMA_HandleTypeDef *_hdma)
{
	uint32_t data_size = sizeof(rx_buff) - __HAL_DMA_GET_COUNTER(&hdma_usart2_rx);
	if( data_size > 0 )
	{
		SerialCommands_ReceiveMessage(&command_ctx, (const char*)rx_buff, data_size);
	}
	HAL_UART_AbortReceive(&huart2);
	HAL_UART_Receive_DMA(&huart2, rx_buff, sizeof(rx_buff));
}

void UART2IdleCallback()
{
	uart_dma_transfer_complete(&hdma_usart2_rx);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
