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
#include "dma.h"
#include "iwdg.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stm_flash.h"
#include "ftp.h"
#include "bsp_iap.h"
#include "md5.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
//寄存器版本
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE {
    HAL_UART_Transmit(&huart2, (uint8_t *) &ch, 1, 0xffff);
    return ch;
}

/******************************************************************************
  * @函数名称:   CloseIQHard
  * @函数说明   :IAP 关掉中断以及硬件外设
******************************************************************************/
void CloseIQHard(void) {
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    __disable_irq();   // 关闭总中断
    //关中断
}
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
enum {
    OTA_INIT = 0,
    OTA_CONFIG,
    OTA_LOGIN,
    OTA_DOWNLOAD,
    OTA_CHECK,
    OTA_WRITE,
    OTA_JUMP,
} OTA_STATUS;//注意，如果升级后程序跑不正常，重刷机
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
    int i = 0;
    static uint8_t restartCount = 0;
    static uint8_t errorCount = 0;
    static uint8_t openCount = 0;//开机失败次数加1
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_IWDG_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    HAL_UART_Receive_DMA(&huart1, lteRxBuf, LTE_USART_REC_LEN);//防止分段

    printf("boot loder\r\n");
    HAL_Delay(200);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

    get_app_infomation(&mcuOtaData);
    if (mcuOtaData.upDataFlag == 1) {
        printf("ota begin\r\n");
        OTA_STATUS = OTA_INIT;
    } else {
        printf("no ota\r\n");
        OTA_STATUS = OTA_JUMP;
    }
		
    while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        //异常处理机制
        if (openCount > 3) {
            openCount = 0;
            OTA_STATUS = OTA_JUMP;//没电进入app程序,等待升级
        }

        if (restartCount > 3) {
            restartCount = 0;
            OTA_STATUS = OTA_JUMP;//升级失败，直接跳转
        }

        if (errorCount > 3) {
            errorCount = 0;
            //是否需要关机？
            ++restartCount;//重启次数加1
            ftpserver_logout();
            ModuleClose();
            HAL_Delay(5000);
            OTA_STATUS = OTA_INIT;
        }

        switch (OTA_STATUS) {
            case OTA_INIT:
                if (MQTTClient_init() == 0) {
                    OTA_STATUS = OTA_CONFIG;
                } else {
                    openCount++;//没电时可能频繁开机失败
                }
                break;
            case OTA_CONFIG:
                if (ftpserver_config(&mcuOtaData) == 0) {
                    printf("config ok\r\n");
                    OTA_STATUS = OTA_LOGIN;
                } else {
                    errorCount++;
                }
                break;
            case OTA_LOGIN:
                if (ftpserver_login(&mcuOtaData) == 0) {
                    printf("login ok\r\n");
                    OTA_STATUS = OTA_DOWNLOAD;
                } else {
                    errorCount++;
                }
                break;

            case OTA_DOWNLOAD:
                if (downloadAndWrite() == 0) {
                    printf("download ok\r\n");
                    OTA_STATUS = OTA_CHECK;
                } else {
                    errorCount++;
                }
                break;
            case OTA_CHECK:
                if(getMd5AndCmp((unsigned char *)FLASH_FileAddress,mcuFileData.app_size,decrypt)==0){
                    OTA_STATUS = OTA_WRITE;
                }else {
                    for(i=0;i<16;i++)//md5校验结果计算
                    {
                       printf("%x\r\n",decrypt[i]);
                    }
                    errorCount++;
                }
                break;
            case OTA_WRITE:
							__disable_irq();
                STMFLASH_Write(FLASH_AppAddress, (uint16_t *) FLASH_FileAddress, mcuFileData.app_size/2+mcuFileData.app_size%2);
						__enable_irq();
                OTA_STATUS = OTA_JUMP;
                if(getMd5AndCmp((unsigned char *)FLASH_AppAddress,mcuFileData.app_size,decrypt)==0){
                    printf("check again!!!\r\n");
                }
                ftpserver_logout();
                break;
                //再次校验加入
            case OTA_JUMP:
								__disable_irq();
                HAL_DeInit();
                HAL_UART_MspDeInit(&huart1);
                HAL_UART_MspDeInit(&huart2);
                IAP_ExecuteApp(FLASH_AppAddress);
                break;
            default:
                break;
        }
        HAL_IWDG_Refresh(&hiwdg);
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* USART1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/* USER CODE BEGIN 4 */

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
