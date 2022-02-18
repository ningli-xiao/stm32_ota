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
    char serverAddress[20];
    uint16_t serverPort;
    char userName[20];
    char password[20];
}OtaData;
/************************** IAP �ⲿ����********************************/
#define APP_FLASH_LEN  		20480       //���� APP �̼��������20kB=20*1024=20480

/* ��չ���� ------------------------------------------------------------------*/

/* �������� ------------------------------------------------------------------*/
extern void IAP_Write_App_Bin( uint32_t appxaddr, uint8_t * appbuf, uint32_t applen);	//��ָ����ַ��ʼ,д��bin
extern void IAP_ExecuteApp( uint32_t appxaddr );			                              //ִ��flash�����app����
extern void get_app_infomation(OtaData* otaInfo);
#endif /* __IAP_H__ */
