/**
  ******************************************************************************
  * �ļ�����: bsp_iap.c 
  * ��    ��: sylincom�����Ŷ�
  * ��    ��: V1.0
  * ��д����: 2015-10-04
  * ��    ��: IAP�ײ�����ʵ��
  ******************************************************************************
  */
/* ����ͷ�ļ� ----------------------------------------------------------------*/
#include <stm32f10x.h>
#include <stdio.h>
#include "bsp/IAP/bsp_iap.h"
#include "bsp/stmflash/stm_flash.h"
#include "bsp/spi/bsp_spi_flash.h"

/* ˽�����Ͷ��� --------------------------------------------------------------*/
/* ˽�к궨�� ----------------------------------------------------------------*/
/* ˽�б��� ------------------------------------------------------------------*/
/************************************************************
** ��������:   IAP_Write_App_Bin
** ��������: ��SPI��ȡ�ļ���д�뵽STM32FLASH��
** �� �� ֵ:
** ��     ��:qifei
** ��     ��:2020.8.05
**************************************************************/
void IAP_Write_App_Bin ( uint32_t ulStartAddr, uint8_t * pBin_DataBuf, uint32_t ulBufLength )
{
	uint16_t us=0;
	uint16_t pack_num=0;
	uint16_t rest_len=0;
	uint32_t ulAdd_Write = ulStartAddr;                                //��ǰд��ĵ�ַ
	uint8_t * pData = pBin_DataBuf;
	
	rest_len=ulBufLength%Split_LEN;
	//�ոպ�����
	if(rest_len ==0){
		pack_num = ulBufLength / Split_LEN;
	}
	else{
		pack_num = ulBufLength / Split_LEN +1;
	}
	
	for ( us = 0; us < (pack_num-1); us ++ )
	{
		SPI_FLASH_BufferRead(pData,SPI_FLASH_WriteAddress+us*Split_LEN,Split_LEN);//��ȡSplit_LEN���ֽ�
		STMFLASH_Write ( ulAdd_Write,(u16*) pData, Split_LEN/2 );	
		ulAdd_Write += Split_LEN;                                           //ƫ��2048  16=2*8.����Ҫ����2.
	}
	
	if (0!=rest_len) 
	{
			SPI_FLASH_BufferRead(pData,SPI_FLASH_WriteAddress+(pack_num-1)*Split_LEN,rest_len);//��ȡSplit_LEN���ֽ�
    	STMFLASH_Write ( ulAdd_Write, (u16*)pData, (rest_len/2+rest_len%2) );//������һЩ�����ֽ�д��ȥ. 
   }
}

__asm void MSR_MSP ( uint32_t ulAddr ) 
{
    MSR MSP, r0 			                   //set Main Stack value
    BX r14
}


//��ת��Ӧ�ó����
//ulAddr_App:�û�������ʼ��ַ.
void IAP_ExecuteApp ( uint32_t ulAddr_App )
{
	pIapFun_TypeDef pJump2App; 
	
	if ( ( ( * ( vu32 * ) ulAddr_App ) & 0x2FFE0000 ) == 0x20000000 )	  //���ջ����ַ�Ƿ�Ϸ�.
	{ 
		pJump2App = ( pIapFun_TypeDef ) * ( vu32 * ) ( ulAddr_App + 4 );	//�û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)		
		MSR_MSP ( * ( vu32 * ) ulAddr_App );					                    //��ʼ��APP��ջָ��(�û��������ĵ�һ�������ڴ��ջ����ַ)
		pJump2App ();								                                    	//��ת��APP.
	}
}		

/******************* (C) COPYRIGHT 2015-2020 ӲʯǶ��ʽ�����Ŷ� *****END OF FILE****/
