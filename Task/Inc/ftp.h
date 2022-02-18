#ifndef __BSP_DEBUG_USART_H__
#define __BSP_DEBUG_USART_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f0xx_hal.h"
#include <stdio.h>
/* 类型定义 ------------------------------------------------------------------*/
/* 宏定义 --------------------------------------------------------------------*/

/* 扩展变量 ------------------------------------------------------------------*/	
#define DOWNLOADLEN	1024
#define LTE_USART_REC_LEN  			DOWNLOADLEN+256 	//定义最大接收字节数下载长度加上256字节


extern uint8_t Lte_RX_BUF[LTE_USART_REC_LEN] ;

extern uint32_t  app_size;//bin文件大小
extern uint16_t app_version;//版本号
extern uint8_t updata_flag;//升级标志
extern uint8_t app_md5[17];//MD5值
extern uint8_t error_count;
extern char handle[10];

/* 函数声明 ------------------------------------------------------------------*/
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

/******************* (C) COPYRIGHT 2020 sylincom开发团队 *****END OF FILE****/
