#ifndef __STMFLASH_H__
#define __STMFLASH_H__

/* ����ͷ�ļ� ----------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* ���Ͷ��� ------------------------------------------------------------------*/
/* �궨�� --------------------------------------------------------------------*/
/************************** STM32 �ڲ� FLASH ���� *****************************/
#define STM32_FLASH_SIZE 128  //��ѡSTM32��FLASH������С(��λΪK)
#define STM32_FLASH_WREN 1    //ʹ�� FLASH д��(0��������;1��ʹ��)

//!!!��Ϊ�忨û�����spi����˴������25k
#define FLASH_AppAddress 0x8003000 //app��ʼ��ַ��12k��ʼ
#define FLASH_FileAddress 0x8009400 //app��ʼ��ַ��37��ʼ
#define FLASH_InfoAddress 0x800FC00 //�洢������Ϣ��63k��ʼ

/* ��չ���� ------------------------------------------------------------------*/
extern uint16_t OtaData_Buffer[20];
/* �������� ------------------------------------------------------------------*/
uint8_t STMFLASH_Readbyte ( uint32_t faddr );
uint16_t STMFLASH_ReadHalfWord(uint32_t faddr);		  //��������  

void STMFLASH_Write( uint32_t WriteAddr, uint16_t * pBuffer, uint16_t NumToWrite );		//��ָ����ַ��ʼд��ָ�����ȵ�����
void STMFLASH_Read( uint32_t ReadAddr, uint16_t * pBuffer, uint16_t NumToRead );   	//��ָ����ַ��ʼ����ָ�����ȵ�����

void Test_Write(uint32_t WriteAddr, uint16_t * WriteData,uint16_t NumToWrite);

#endif /* __STMFLASH_H__ */

/******************* (C) COPYRIGHT 2015-2020 ӲʯǶ��ʽ�����Ŷ� *****END OF FILE****/
