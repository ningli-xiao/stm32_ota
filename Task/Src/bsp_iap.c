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

/* ˽�����Ͷ��� --------------------------------------------------------------*/
/* ˽�к궨�� ----------------------------------------------------------------*/
/* ˽�б��� ------------------------------------------------------------------*/

/*******************************************************************************
* Function Name  : get_app_infomation
* Description    : ��ȡ��������������,��ȡ������������Ϣ
* Input          : None
* Input          : None
* Output         : None
* Return         : TRUE (success) / FALSE (error)
*******************************************************************************/
void get_app_infomation(OtaData *otaInfo) {
    uint16_t OtaData_Buffer[5] = {0};
    //STMFLASH��ȡ20���ֽ�
    STMFLASH_Read(FLASH_InfoAddress, (uint16_t *) OtaData_Buffer, 2);//��ȡ1*2=2���ֽ�
    //�洢�汾��,�����С��MD5ֵ

    otaInfo->upDataFlag = OtaData_Buffer[0];
    otaInfo->appVersion = OtaData_Buffer[1];
    //memcpy(otaInfo->serverAddress,OtaData_Buffer[],);
    //�ж���ɺ�����
    OtaData_Buffer[0] = 0xffff;
    OtaData_Buffer[1] = 0xffff;
    __disable_irq();
    STMFLASH_Write(FLASH_InfoAddress, (uint16_t *) &OtaData_Buffer[0], 2);//дһ������
    __enable_irq();
//    STMFLASH_Read(FLASH_InfoAddress,(uint16_t*)OtaData_Buffer,2);//��ȡ1*2=2���ֽ�
//    printf("otadata��%x,%x",OtaData_Buffer[0],OtaData_Buffer[1]);
}

/************************************************************
** ��������:   IAP_Write_App_Bin
** ��������: ��SPI��ȡ�ļ���д�뵽STM32FLASH��
** �� �� ֵ:
**************************************************************/
void IAP_Write_App_Bin(uint32_t ulStartAddr, uint8_t *pBin_DataBuf, uint32_t ulBufLength) {
    uint16_t us = 0;
    uint16_t pack_num = 0;
    uint16_t rest_len = 0;
    uint32_t ulAdd_Write = ulStartAddr;                                //��ǰд��ĵ�ַ
    uint8_t *pData = pBin_DataBuf;

    rest_len = ulBufLength % Split_LEN;
    //�ոպ�����
    if (rest_len == 0) {
        pack_num = ulBufLength / Split_LEN;
    } else {
        pack_num = ulBufLength / Split_LEN + 1;
    }

    for (us = 0; us < (pack_num - 1); us++) {
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

void IAP_ExecuteApp(uint32_t ulAddr_App)
{
    pIapFun_TypeDef pJump2App;
    if (((*(uint32_t *) ulAddr_App) & 0x2FFE0000) == 0x20000000)      //���ջ����ַ�Ƿ�Ϸ�.
    {
        pJump2App = (pIapFun_TypeDef) *(uint32_t *) (ulAddr_App + 4);    //�û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)
        MSR_MSP(*(uint32_t *) ulAddr_App);                                        //��ʼ��APP��ջָ��(�û��������ĵ�һ�������ڴ��ջ����ַ)
        pJump2App();                                                                        //��ת��APP.
    }
}

/******************* (C) COPYRIGHT 2015-2020 ӲʯǶ��ʽ�����Ŷ� *****END OF FILE****/
