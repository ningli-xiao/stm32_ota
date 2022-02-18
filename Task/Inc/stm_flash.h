#ifndef __STMFLASH_H__
#define __STMFLASH_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* 类型定义 ------------------------------------------------------------------*/
/* 宏定义 --------------------------------------------------------------------*/
/************************** STM32 内部 FLASH 配置 *****************************/
#define STM32_FLASH_SIZE 128  //所选STM32的FLASH容量大小(单位为K)
#define STM32_FLASH_WREN 1    //使能 FLASH 写入(0，不是能;1，使能)

//!!!因为板卡没有设计spi，因此代码最多25k
#define FLASH_AppAddress 0x8003000 //app起始地址，12k开始
#define FLASH_FileAddress 0x8009400 //app起始地址，37开始
#define FLASH_InfoAddress 0x800FC00 //存储升级信息，63k开始

/* 扩展变量 ------------------------------------------------------------------*/
extern uint16_t OtaData_Buffer[20];
/* 函数声明 ------------------------------------------------------------------*/
uint8_t STMFLASH_Readbyte ( uint32_t faddr );
uint16_t STMFLASH_ReadHalfWord(uint32_t faddr);		  //读出半字  

void STMFLASH_Write( uint32_t WriteAddr, uint16_t * pBuffer, uint16_t NumToWrite );		//从指定地址开始写入指定长度的数据
void STMFLASH_Read( uint32_t ReadAddr, uint16_t * pBuffer, uint16_t NumToRead );   	//从指定地址开始读出指定长度的数据

void Test_Write(uint32_t WriteAddr, uint16_t * WriteData,uint16_t NumToWrite);

#endif /* __STMFLASH_H__ */

/******************* (C) COPYRIGHT 2015-2020 硬石嵌入式开发团队 *****END OF FILE****/
