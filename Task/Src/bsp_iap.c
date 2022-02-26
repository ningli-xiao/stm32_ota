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

/************************************************************
** ��������:   IAP_Write_App_Bin
** ��������: ���ļ���������ȡ�ļ�д�뵽APP��
** �� �� ֵ:
**************************************************************/
void IAP_Write_App_Bin(uint32_t ulStartAddr, uint16_t *pBin_DataBuf, uint32_t ulBufLength) {
    uint16_t us = 0;
    uint16_t pack_num = 0;
    uint16_t rest_len = 0;
    uint32_t ulAdd_Write = ulStartAddr;                                //��ǰд��ĵ�ַ
    uint16_t *pData = pBin_DataBuf;

    rest_len = ulBufLength % Split_LEN;
    //�ոպ�����
    if (rest_len == 0) {
        pack_num = ulBufLength / Split_LEN;
    } else {
        pack_num = ulBufLength / Split_LEN + 1;
    }

    for (us = 0; us < (pack_num - 1); us++) {

//        memset(MD5bin,0,DOWNLOADLEN);
//        STMFLASH_Read(pData,MD5bin,DOWNLOADLEN/2);

        STMFLASH_Write(ulAdd_Write, (uint16_t *) pData, Split_LEN / 2);
        ulAdd_Write += Split_LEN;                                           //ƫ��2048  16=2*8.����Ҫ����2.
    }

    if (0 != rest_len) {
        STMFLASH_Write(ulAdd_Write, (uint16_t *) pData, (rest_len / 2 + rest_len % 2));//������һЩ�����ֽ�д��ȥ.
    }
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
