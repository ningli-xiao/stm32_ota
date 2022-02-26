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
#include "stm32f0xx_hal.h"
#include <stdio.h>
#include <string.h>
#include "bsp_iap.h"
#include "stm_flash.h"
#include "ftp.h"
#include "md5.h"
/* ˽�����Ͷ��� --------------------------------------------------------------*/
/* ˽�к궨�� ----------------------------------------------------------------*/
/* ˽�б��� ------------------------------------------------------------------*/
OtaData mcuOtaData;
FileData mcuFileData;
uint8_t MD5bin[1024]={0};
/*
 * ��������MQTT_Comma_Pos
 * ���ܣ�����buffer���ָ��λ������
 * ���룺buf:�������ݵ�ַ��cx:�ڼ����ָ���
 * ���أ���ַ��ֵ
 */
uint8_t MQTT_Comma_Pos(uint8_t *buf, uint8_t cx) {
    uint8_t *p = buf;
    while (cx) {
        if (*buf == '*' || *buf < ' ' || *buf > 'z')return 0XFF;//����'*'���߷Ƿ��ַ�,�򲻴��ڵ�cx������
        if (*buf == ',')cx--;
        buf++;
    }
    return buf - p;   //���ز�ֵ��
}

/*******************************************************************************
* Function Name  : get_app_infomation
* Description    : ��ȡ��������������,��ȡ������������Ϣ
* Input          : None
* Input          : None
* Output         : None
* Return         : TRUE (success) / FALSE (error)
*******************************************************************************/
int get_app_infomation(OtaData *otaInfo) {
    uint8_t *ptr = NULL;
    uint8_t posX = 0;//cell����λ��
    uint8_t posY = 0;//cell��һ������λ��
    char  temp_Buffer[100] = {0};

    STMFLASH_Read(FLASH_InfoAddress, (uint16_t *) temp_Buffer, 50);//��ȡ1*2=2���ֽ�

    ptr = strstr(temp_Buffer, "update");
    if (ptr == 0) {
        return -1;
    }
    otaInfo->upDataFlag = 1;

    //��ȡ��Ϣ���ַ���
    posX = MQTT_Comma_Pos(ptr, 1);
    posY = MQTT_Comma_Pos(ptr, 2);
    if (posX != 0XFF && posY != 0XFF) {
        memcpy(otaInfo->serverAddress, ptr + posX, (posY - posX - 1));
        printf("server address is:%s\r\n", otaInfo->serverAddress);
    }

    posX = MQTT_Comma_Pos(ptr, 2);
    posY = MQTT_Comma_Pos(ptr, 3);
    if (posX != 0XFF && posY != 0XFF) {
        memcpy(otaInfo->serverPort, ptr + posX, (posY - posX - 1));
        printf("port is:%s\r\n", otaInfo->serverPort);
    }

    posX = MQTT_Comma_Pos(ptr, 3);
    posY = MQTT_Comma_Pos(ptr, 4);
    if (posX != 0XFF && posY != 0XFF) {
        memcpy(otaInfo->userName, ptr + posX, (posY - posX - 1));
        printf("userName is:%s\r\n", otaInfo->userName);
    }

    posX = MQTT_Comma_Pos(ptr, 4);
    posY = MQTT_Comma_Pos(ptr, 5);
    if (posX != 0XFF && posY != 0XFF) {
        memcpy(otaInfo->password, ptr + posX, (posY - posX - 1));
        printf("password is:%s\r\n", otaInfo->password);
    }

    posX = MQTT_Comma_Pos(ptr, 5);
    posY = MQTT_Comma_Pos(ptr, 6);
    if (posX != 0XFF && posY != 0XFF) {
        memcpy(otaInfo->fileName, ptr + posX, (posY - posX - 1));
        printf("fileName is:%s\r\n", otaInfo->fileName);
    }

    memset(temp_Buffer,0x56,100);//���buffer

    __disable_irq();
    printf("run write\r\n");
    STMFLASH_Write(FLASH_InfoAddress, (uint16_t *) &temp_Buffer[0], 50);//дһ������
    printf("run end\r\n");
    __enable_irq();
    return 0;
}




__ASM void MSR_MSP(uint32_t addr)
{
	MSR MSP, r0
	BX r14
}

void IAP_ExecuteApp(uint32_t ulAddr_App) {
    pIapFun_TypeDef pJump2App;
    if (((*(uint32_t *) ulAddr_App) & 0x2FFE0000) == 0x20000000)      //���ջ����ַ�Ƿ�Ϸ�.
    {
        pJump2App = (pIapFun_TypeDef) *(uint32_t *) (ulAddr_App + 4);    //�û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)
        MSR_MSP(*(uint32_t *) ulAddr_App);                                        //��ʼ��APP��ջָ��(�û��������ĵ�һ�������ڴ��ջ����ַ)
        pJump2App();                                                                        //��ת��APP.
    }
}

/******************* (C) COPYRIGHT 2015-2020 ӲʯǶ��ʽ�����Ŷ� *****END OF FILE****/
