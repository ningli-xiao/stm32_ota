#ifndef __BSP_DEBUG_USART_H__
#define __BSP_DEBUG_USART_H__

/* ����ͷ�ļ� ----------------------------------------------------------------*/
#include "stm32f0xx_hal.h"
#include <stdio.h>
#include "bsp_iap.h"
/* ���Ͷ��� ------------------------------------------------------------------*/
/* �궨�� --------------------------------------------------------------------*/

/* ��չ���� ------------------------------------------------------------------*/	
#define DOWNLOADLEN	1024
#define LTE_USART_REC_LEN  			DOWNLOADLEN+256 	//�����������ֽ������س��ȼ���256�ֽ�

extern uint8_t lteRxFlag;
extern uint8_t lteRxBuf[LTE_USART_REC_LEN] ;
extern char handle[10];

/* �������� ------------------------------------------------------------------*/
extern void SoftReset(void);
extern int Wait_LTE_RDY(uint8_t time);
extern int Wait_Signal_RDY(uint8_t time);
extern char ModuleOpen(void);
extern int MQTTClient_init();

extern char  downloadAndWrite();
extern char deletefile();
extern char downloadfile(OtaData *otaInfo);
extern char writefile(uint32_t len,char* FileHandle);
extern char ftpserver_config(OtaData *ftpInfo);
extern char ftpserver_login(OtaData *ftpInfo);
extern char ftpserver_logout();
extern uint32_t getfile_size(char *fileName);
extern char getfile_headmd5(FileData *fileInfo,char *fileName);


//void vFtpTask(void);


#endif  

/******************* (C) COPYRIGHT 2020 sylincom�����Ŷ� *****END OF FILE****/
