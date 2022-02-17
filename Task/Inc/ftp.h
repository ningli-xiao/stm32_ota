#ifndef __BSP_DEBUG_USART_H__
#define __BSP_DEBUG_USART_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include <stm32f10x.h>
#include <stdio.h>

/* 类型定义 ------------------------------------------------------------------*/

/* 宏定义 --------------------------------------------------------------------*/
	#define LTE_USART3                                 USART3
	#define LTE_USART3_ClockCmd                        RCC_APB1PeriphClockCmd
	#define LTE_USART3_CLK                             RCC_APB1Periph_USART3

	#define LTE_USART3_GPIO_ClockCmd                   RCC_APB2PeriphClockCmd    
	#define LTE_USART3_TX_PORT                         GPIOC 
	#define LTE_USART3_TX_PIN                          GPIO_Pin_10
	#define LTE_USART3_TX_CLK                          RCC_APB2Periph_GPIOC

	#define LTE_USART3_RX_PORT                         GPIOC 
	#define LTE_USART3_RX_PIN                          GPIO_Pin_11
	#define LTE_USART3_RX_CLK                          RCC_APB2Periph_GPIOC

	#define LTE_USART3_IRQHANDLER                      USART3_IRQHandler
	#define LTE_USART3_IRQn                            USART3_IRQn

	#define IO_LTE_ACTIVATION_CONTROL_PIN 			   GPIO_Pin_8   
	#define IO_LTE_ACTIVATION_CONTROL_PORT			   GPIOA


	#define IO_WAKE_Control_PIN 			GPIO_Pin_5 
	#define IO_WAKE_Control_PORT			GPIOB

	#define IO_EN_VBAT_Control_PIN      			   GPIO_Pin_1
	#define IO_EN_VBAT_VCC_Control_PORT    			   GPIOC

	#define RS485_DE_PORT        			GPIOB   
	#define RS485_DE_PIN         			GPIO_Pin_4

	#define RS485_RE_PORT        			GPIOD  
	#define RS485_RE_PIN         			GPIO_Pin_2
/* 扩展变量 ------------------------------------------------------------------*/	
#define DOWNLOADLEN	1024
#define LTE_USART_REC_LEN  			DOWNLOADLEN+256 	//定义最大接收字节数下载长度加上256字节

struct s_byte32
{
    u32 byte3:8;
    u32 byte2:8;
    u32 byte1:8;
    u32 byte0:8;
};
struct s_bytestr
{
    u8 byte_str[4];
};
union u_byte_int
{
    struct s_byte32 byte32;
    int data_int;
};

union u_byte_u32
{
    struct s_byte32 byte32;
    struct s_bytestr bytestr;
    u32 data_u32;
};
union u_u32_float
{
    u32 u_int32;
    float float_32;
};

typedef enum
{
    GPIO_DATA_LOW,
    GPIO_DATA_HIGH
} GPIO_DataTy;

extern u32 app_size;//bin文件大小
extern u16 app_version;//版本号
extern u8 updata_flag;//升级标志
extern u8 app_md5[17];//MD5值
extern u8 error_count;
extern char handle[10];

/* 函数声明 ------------------------------------------------------------------*/
void SoftReset(void);
void Lte_USART3_Init(u32 bound);
void Usart3_SendByte(char ch);
void Usart3_SendString(char *str);
void Usart3_SendStr_length(uint8_t *str,uint32_t strlen);
void LTE_UART_SendArrayData(u8 *pdatabuf,u32 nbchar );
void LTE_UART_SendData(char *pdatabuf);
void LTE_GPIO_Init(void);

int Wait_LTE_RDY(u8 time);
int Wait_Signal_RDY(u8 time);
void LteActivation(void);
char* Send_FTPATCommand(char *pCommand, char *pEcho, u32 outTime, u32 *return_len);
char deletefile();
char downloadfile();
char writefile(u32 len);
char ftpserver_config();
char ftpserver_login();
u32 getfile_size();
u32 getfile_list();
char getfile_headmd5(u8* md5);


//void vFtpTask(void);


#endif  

/******************* (C) COPYRIGHT 2020 sylincom开发团队 *****END OF FILE****/
