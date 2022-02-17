/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define IO10_Pin GPIO_PIN_13
#define IO10_GPIO_Port GPIOC
#define IO9_Pin GPIO_PIN_14
#define IO9_GPIO_Port GPIOC
#define IO3_Pin GPIO_PIN_15
#define IO3_GPIO_Port GPIOC
#define IO6_Pin GPIO_PIN_0
#define IO6_GPIO_Port GPIOF
#define IO7_Pin GPIO_PIN_1
#define IO7_GPIO_Port GPIOF
#define IO16_Pin GPIO_PIN_0
#define IO16_GPIO_Port GPIOA
#define RST_EC200_Pin GPIO_PIN_1
#define RST_EC200_GPIO_Port GPIOA
#define AT_TX_Pin GPIO_PIN_2
#define AT_TX_GPIO_Port GPIOA
#define AT_RX_Pin GPIO_PIN_3
#define AT_RX_GPIO_Port GPIOA
#define IO15_Pin GPIO_PIN_4
#define IO15_GPIO_Port GPIOA
#define IO14_Pin GPIO_PIN_5
#define IO14_GPIO_Port GPIOA
#define IO13_Pin GPIO_PIN_6
#define IO13_GPIO_Port GPIOA
#define IO2_Pin GPIO_PIN_7
#define IO2_GPIO_Port GPIOA
#define ADC1_Pin GPIO_PIN_0
#define ADC1_GPIO_Port GPIOB
#define ADC2_Pin GPIO_PIN_1
#define ADC2_GPIO_Port GPIOB
#define IO12_Pin GPIO_PIN_10
#define IO12_GPIO_Port GPIOB
#define IO11_Pin GPIO_PIN_11
#define IO11_GPIO_Port GPIOB
#define ONOFF_EC200_Pin GPIO_PIN_8
#define ONOFF_EC200_GPIO_Port GPIOA
#define SPI_CS1_Pin GPIO_PIN_9
#define SPI_CS1_GPIO_Port GPIOA
#define SPI_CS2_Pin GPIO_PIN_10
#define SPI_CS2_GPIO_Port GPIOA
#define SPI_CS3_Pin GPIO_PIN_11
#define SPI_CS3_GPIO_Port GPIOA
#define IO_0_Pin GPIO_PIN_12
#define IO_0_GPIO_Port GPIOA
#define IO1_Pin GPIO_PIN_15
#define IO1_GPIO_Port GPIOA
#define IO4_Pin GPIO_PIN_3
#define IO4_GPIO_Port GPIOB
#define IO5_Pin GPIO_PIN_4
#define IO5_GPIO_Port GPIOB
#define IO8_Pin GPIO_PIN_5
#define IO8_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
