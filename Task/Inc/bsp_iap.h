#ifndef __IAP_H__
#define	__IAP_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include <stm32f10x.h>
#define Split_LEN 1024
/* 类型定义 ------------------------------------------------------------------*/
/************************** IAP 数据类型定义********************************/
typedef  void ( * pIapFun_TypeDef ) ( void ); //定义一个函数类型的参数.

/* 宏定义 --------------------------------------------------------------------*/
/************************** IAP 宏参数定义********************************/                                    
#define APP_START_ADDR       0x8004000  	//应用程序起始地址(存放在FLASH)
#define OTA_DATA_ADDR        0x801E800  	//升级信息存储地址
#define TEST_DATA_ADDR       0x801FC00  	//测试信息存储地址
//#define APP_START_ADDR      0x20001000  	//应用程序起始地址(存放在RAM)


/************************** IAP 外部变量********************************/
#define APP_FLASH_LEN  		40960       //定义 APP 固件最大容量20kB=20*1024=20480

/* 扩展变量 ------------------------------------------------------------------*/

/* 函数声明 ------------------------------------------------------------------*/
void IAP_Write_App_Bin( uint32_t appxaddr, uint8_t * appbuf, uint32_t applen);	//在指定地址开始,写入bin
void IAP_ExecuteApp( uint32_t appxaddr );			                              //执行flash里面的app程序

#endif /* __IAP_H__ */
