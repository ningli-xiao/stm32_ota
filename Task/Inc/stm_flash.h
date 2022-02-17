#ifndef __STMFLASH_H__
#define __STMFLASH_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f10x.h"

/* 类型定义 ------------------------------------------------------------------*/
/* 宏定义 --------------------------------------------------------------------*/
/************************** STM32 内部 FLASH 配置 *****************************/
#define STM32_FLASH_SIZE 128  //所选STM32的FLASH容量大小(单位为K)
#define STM32_FLASH_WREN 1    //使能 FLASH 写入(0，不是能;1，使能)
#define FLASH_WriteAddress 0x800FC00 
/* 扩展变量 ------------------------------------------------------------------*/
extern u16 OtaData_Buffer[20];
/* 函数声明 ------------------------------------------------------------------*/
uint8_t STMFLASH_Readbyte ( uint32_t faddr );
uint16_t STMFLASH_ReadHalfWord(uint32_t faddr);		  //读出半字  
void STMFLASH_WriteLenByte(uint32_t WriteAddr, uint32_t DataToWrite, uint16_t Len );	      //指定地址开始写入指定长度的数据
uint32_t STMFLASH_ReadLenByte(uint32_t ReadAddr, uint16_t Len );					                    	//指定地址开始读取指定长度数据
void STMFLASH_Write( uint32_t WriteAddr, uint16_t * pBuffer, uint16_t NumToWrite );		//从指定地址开始写入指定长度的数据
void STMFLASH_Read( uint32_t ReadAddr, uint16_t * pBuffer, uint16_t NumToRead );   	//从指定地址开始读出指定长度的数据
void STMFLASH_Read_ota( uint32_t ReadAddr, uint8_t * pBuffer, uint16_t NumToRead ); 
void Test_Write( uint32_t WriteAddr, uint16_t WriteData );                        //测试写入		

#endif /* __STMFLASH_H__ */

/******************* (C) COPYRIGHT 2015-2020 硬石嵌入式开发团队 *****END OF FILE****/
