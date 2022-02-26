#ifndef __IAP_H__
#define	__IAP_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f0xx_hal.h"
#define Split_LEN 1024
/* 类型定义 ------------------------------------------------------------------*/
/************************** IAP 数据类型定义********************************/
typedef  void ( * pIapFun_TypeDef ) ( void ); //定义一个函数类型的参数.
/* 宏定义 --------------------------------------------------------------------*/
/************************** IAP 宏参数定义********************************/                                    
typedef struct {
    uint8_t upDataFlag;
    uint16_t appVersion;
    char fileName[20];
    char serverAddress[20];
    char serverPort[5];
    char userName[20];
    char password[20];
}OtaData;

typedef struct {
    uint32_t app_size;
    char handle[10];
    uint8_t md5[16];
}FileData;

/************************** IAP 外部变量********************************/
#define APP_FLASH_LEN  		20480       //定义 APP 固件最大容量20kB=20*1024=20480

/* 扩展变量 ------------------------------------------------------------------*/
extern OtaData mcuOtaData;
extern FileData mcuFileData;
extern uint8_t MD5bin[1024];
/* 函数声明 ------------------------------------------------------------------*/
extern void IAP_ExecuteApp( uint32_t appxaddr);			                              //执行flash里面的app程序
extern int get_app_infomation(OtaData* otaInfo);
#endif /* __IAP_H__ */
