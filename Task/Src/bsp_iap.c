/**
  ******************************************************************************
  * 文件名程: bsp_iap.c 
  * 作    者: sylincom开发团队
  * 版    本: V1.0
  * 编写日期: 2015-10-04
  * 功    能: IAP底层驱动实现
  ******************************************************************************
  */
/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f0xx_hal.h"
#include <stdio.h>
#include <string.h>
#include "bsp_iap.h"
#include "stm_flash.h"
#include "ftp.h"
#include "md5.h"
/* 私有类型定义 --------------------------------------------------------------*/
/* 私有宏定义 ----------------------------------------------------------------*/
/* 私有变量 ------------------------------------------------------------------*/
OtaData mcuOtaData;
FileData mcuFileData;
uint8_t MD5bin[1024]={0};
/*
 * 函数名：MQTT_Comma_Pos
 * 功能：查找buffer里的指定位置数据
 * 输入：buf:接受数据地址，cx:第几个分隔符
 * 返回：地址差值
 */
uint8_t MQTT_Comma_Pos(uint8_t *buf, uint8_t cx) {
    uint8_t *p = buf;
    while (cx) {
        if (*buf == '*' || *buf < ' ' || *buf > 'z')return 0XFF;//遇到'*'或者非法字符,则不存在第cx个逗号
        if (*buf == ',')cx--;
        buf++;
    }
    return buf - p;   //返回差值，
}

/*******************************************************************************
* Function Name  : get_app_infomation
* Description    : 读取升级备份区数据,获取升级包基础信息
* Input          : None
* Input          : None
* Output         : None
* Return         : TRUE (success) / FALSE (error)
*******************************************************************************/
int get_app_infomation(OtaData *otaInfo) {
    uint8_t *ptr = NULL;
    uint8_t posX = 0;//cell逗号位置
    uint8_t posY = 0;//cell下一个逗号位置
    char  temp_Buffer[100] = {0};

    STMFLASH_Read(FLASH_InfoAddress, (uint16_t *) temp_Buffer, 50);//读取1*2=2个字节

    ptr = strstr(temp_Buffer, "update");
    if (ptr == 0) {
        return -1;
    }
    otaInfo->upDataFlag = 1;

    //读取信息到字符串
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

    memset(temp_Buffer,0x56,100);//清空buffer

    __disable_irq();
    printf("run write\r\n");
    STMFLASH_Write(FLASH_InfoAddress, (uint16_t *) &temp_Buffer[0], 50);//写一次数据
    printf("run end\r\n");
    __enable_irq();
    return 0;
}

/************************************************************
** 函数名称:   IAP_Write_App_Bin
** 功能描述: 从文件缓存区读取文件写入到APP区
** 返 回 值:
**************************************************************/
void IAP_Write_App_Bin(uint32_t ulStartAddr, uint16_t *pBin_DataBuf, uint32_t ulBufLength) {
    uint16_t us = 0;
    uint16_t pack_num = 0;
    uint16_t rest_len = 0;
    uint32_t ulAdd_Write = ulStartAddr;                                //当前写入的地址
    uint16_t *pData = pBin_DataBuf;

    rest_len = ulBufLength % Split_LEN;
    //刚刚好整包
    if (rest_len == 0) {
        pack_num = ulBufLength / Split_LEN;
    } else {
        pack_num = ulBufLength / Split_LEN + 1;
    }

    for (us = 0; us < (pack_num - 1); us++) {

//        memset(MD5bin,0,DOWNLOADLEN);
//        STMFLASH_Read(pData,MD5bin,DOWNLOADLEN/2);

        STMFLASH_Write(ulAdd_Write, (uint16_t *) pData, Split_LEN / 2);
        ulAdd_Write += Split_LEN;                                           //偏移2048  16=2*8.所以要乘以2.
    }

    if (0 != rest_len) {
        STMFLASH_Write(ulAdd_Write, (uint16_t *) pData, (rest_len / 2 + rest_len % 2));//将最后的一些内容字节写进去.
    }
}


__ASM void MSR_MSP(uint32_t addr)
{
	MSR MSP, r0
	BX r14
}

void IAP_ExecuteApp(uint32_t ulAddr_App) {
    pIapFun_TypeDef pJump2App;
    if (((*(uint32_t *) ulAddr_App) & 0x2FFE0000) == 0x20000000)      //检查栈顶地址是否合法.
    {
        pJump2App = (pIapFun_TypeDef) *(uint32_t *) (ulAddr_App + 4);    //用户代码区第二个字为程序开始地址(复位地址)
        MSR_MSP(*(uint32_t *) ulAddr_App);                                        //初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
        pJump2App();                                                                        //跳转到APP.
    }
}

/******************* (C) COPYRIGHT 2015-2020 硬石嵌入式开发团队 *****END OF FILE****/
