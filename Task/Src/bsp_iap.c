/**
  ******************************************************************************
  * 文件名程: bsp_iap.c 
  * 作    者: sylincom开发团队
  * 版    本: V1.0
  * 编写日期: 2015-10-04
  * 功    能: IAP底层驱动实现
  ******************************************************************************
  */
/* 包含头文件 ----------------------------------------------------------------*/
#include <stm32f10x.h>
#include <stdio.h>
#include "bsp/IAP/bsp_iap.h"
#include "bsp/stmflash/stm_flash.h"
#include "bsp/spi/bsp_spi_flash.h"

/* 私有类型定义 --------------------------------------------------------------*/
/* 私有宏定义 ----------------------------------------------------------------*/
/* 私有变量 ------------------------------------------------------------------*/
/************************************************************
** 函数名称:   IAP_Write_App_Bin
** 功能描述: 从SPI获取文件，写入到STM32FLASH中
** 返 回 值:
** 作     者:qifei
** 日     期:2020.8.05
**************************************************************/
void IAP_Write_App_Bin ( uint32_t ulStartAddr, uint8_t * pBin_DataBuf, uint32_t ulBufLength )
{
	uint16_t us=0;
	uint16_t pack_num=0;
	uint16_t rest_len=0;
	uint32_t ulAdd_Write = ulStartAddr;                                //当前写入的地址
	uint8_t * pData = pBin_DataBuf;
	
	rest_len=ulBufLength%Split_LEN;
	//刚刚好整包
	if(rest_len ==0){
		pack_num = ulBufLength / Split_LEN;
	}
	else{
		pack_num = ulBufLength / Split_LEN +1;
	}
	
	for ( us = 0; us < (pack_num-1); us ++ )
	{
		SPI_FLASH_BufferRead(pData,SPI_FLASH_WriteAddress+us*Split_LEN,Split_LEN);//读取Split_LEN个字节
		STMFLASH_Write ( ulAdd_Write,(u16*) pData, Split_LEN/2 );	
		ulAdd_Write += Split_LEN;                                           //偏移2048  16=2*8.所以要乘以2.
	}
	
	if (0!=rest_len) 
	{
			SPI_FLASH_BufferRead(pData,SPI_FLASH_WriteAddress+(pack_num-1)*Split_LEN,rest_len);//读取Split_LEN个字节
    	STMFLASH_Write ( ulAdd_Write, (u16*)pData, (rest_len/2+rest_len%2) );//将最后的一些内容字节写进去. 
   }
}

__asm void MSR_MSP ( uint32_t ulAddr ) 
{
    MSR MSP, r0 			                   //set Main Stack value
    BX r14
}


//跳转到应用程序段
//ulAddr_App:用户代码起始地址.
void IAP_ExecuteApp ( uint32_t ulAddr_App )
{
	pIapFun_TypeDef pJump2App; 
	
	if ( ( ( * ( vu32 * ) ulAddr_App ) & 0x2FFE0000 ) == 0x20000000 )	  //检查栈顶地址是否合法.
	{ 
		pJump2App = ( pIapFun_TypeDef ) * ( vu32 * ) ( ulAddr_App + 4 );	//用户代码区第二个字为程序开始地址(复位地址)		
		MSR_MSP ( * ( vu32 * ) ulAddr_App );					                    //初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
		pJump2App ();								                                    	//跳转到APP.
	}
}		

/******************* (C) COPYRIGHT 2015-2020 硬石嵌入式开发团队 *****END OF FILE****/
