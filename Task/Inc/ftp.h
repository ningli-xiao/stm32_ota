#ifndef __BSP_DEBUG_USART_H__
#define __BSP_DEBUG_USART_H__

/* ����ͷ�ļ� ----------------------------------------------------------------*/
#include "stm32f0xx_hal.h"
#include <stdio.h>
/* ���Ͷ��� ------------------------------------------------------------------*/
/* �궨�� --------------------------------------------------------------------*/

/* ��չ���� ------------------------------------------------------------------*/	
#define DOWNLOADLEN	1024
#define LTE_USART_REC_LEN  			DOWNLOADLEN+256 	//�����������ֽ������س��ȼ���256�ֽ�


extern uint8_t Lte_RX_BUF[LTE_USART_REC_LEN] ;

extern uint32_t  app_size;//bin�ļ���С
extern uint16_t app_version;//�汾��
extern uint8_t updata_flag;//������־
extern uint8_t app_md5[17];//MD5ֵ
extern uint8_t error_count;
extern char handle[10];

/* �������� ------------------------------------------------------------------*/
void SoftReset(void);
int Wait_LTE_RDY(uint8_t time);
int Wait_Signal_RDY(uint8_t time);

void LteActivation(void);
char* Send_FTPATCommand(char *pCommand, char *pEcho, uint32_t outTime);
char deletefile();
char downloadfile();
char writefile(uint32_t len);
char ftpserver_config();
char ftpserver_login();
uint32_t getfile_size();
uint32_t getfile_list();
char getfile_headmd5(uint8_t* md5);


//void vFtpTask(void);


#endif  

/******************* (C) COPYRIGHT 2020 sylincom�����Ŷ� *****END OF FILE****/
