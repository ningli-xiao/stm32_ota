#ifndef __IAP_H__
#define	__IAP_H__

/* ����ͷ�ļ� ----------------------------------------------------------------*/
#include "stm32f0xx_hal.h"
#define Split_LEN 1024
/* ���Ͷ��� ------------------------------------------------------------------*/
/************************** IAP �������Ͷ���********************************/
typedef  void ( * pIapFun_TypeDef ) ( void ); //����һ���������͵Ĳ���.
/* �궨�� --------------------------------------------------------------------*/
/************************** IAP ���������********************************/                                    
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

/************************** IAP �ⲿ����********************************/
#define APP_FLASH_LEN  		20480       //���� APP �̼��������20kB=20*1024=20480

/* ��չ���� ------------------------------------------------------------------*/
extern OtaData mcuOtaData;
extern FileData mcuFileData;
extern uint8_t MD5bin[1024];
/* �������� ------------------------------------------------------------------*/
extern void IAP_ExecuteApp( uint32_t appxaddr);			                              //ִ��flash�����app����
extern int get_app_infomation(OtaData* otaInfo);
#endif /* __IAP_H__ */
