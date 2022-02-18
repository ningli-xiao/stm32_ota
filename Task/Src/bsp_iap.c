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

/* 私有类型定义 --------------------------------------------------------------*/
/* 私有宏定义 ----------------------------------------------------------------*/
/* 私有变量 ------------------------------------------------------------------*/

/*******************************************************************************
* Function Name  : get_app_infomation
* Description    : 读取升级备份区数据,获取升级包基础信息
* Input          : None
* Input          : None
* Output         : None
* Return         : TRUE (success) / FALSE (error)
*******************************************************************************/
void get_app_infomation(OtaData *otaInfo) {
    uint16_t OtaData_Buffer[5] = {0};
    //STMFLASH读取20个字节
    STMFLASH_Read(FLASH_InfoAddress, (uint16_t *) OtaData_Buffer, 2);//读取1*2=2个字节
    //存储版本号,代码大小，MD5值

    otaInfo->upDataFlag = OtaData_Buffer[0];
    otaInfo->appVersion = OtaData_Buffer[1];
    //memcpy(otaInfo->serverAddress,OtaData_Buffer[],);
    //判断完成后清零
    OtaData_Buffer[0] = 0xffff;
    OtaData_Buffer[1] = 0xffff;
    __disable_irq();
    STMFLASH_Write(FLASH_InfoAddress, (uint16_t *) &OtaData_Buffer[0], 2);//写一次数据
    __enable_irq();
//    STMFLASH_Read(FLASH_InfoAddress,(uint16_t*)OtaData_Buffer,2);//读取1*2=2个字节
//    printf("otadata：%x,%x",OtaData_Buffer[0],OtaData_Buffer[1]);
}

/************************************************************
** 函数名称:   IAP_Write_App_Bin
** 功能描述: 从SPI获取文件，写入到STM32FLASH中
** 返 回 值:
**************************************************************/
void IAP_Write_App_Bin(uint32_t ulStartAddr, uint8_t *pBin_DataBuf, uint32_t ulBufLength) {
    uint16_t us = 0;
    uint16_t pack_num = 0;
    uint16_t rest_len = 0;
    uint32_t ulAdd_Write = ulStartAddr;                                //当前写入的地址
    uint8_t *pData = pBin_DataBuf;

    rest_len = ulBufLength % Split_LEN;
    //刚刚好整包
    if (rest_len == 0) {
        pack_num = ulBufLength / Split_LEN;
    } else {
        pack_num = ulBufLength / Split_LEN + 1;
    }

    for (us = 0; us < (pack_num - 1); us++) {
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

void IAP_ExecuteApp(uint32_t ulAddr_App)
{
    pIapFun_TypeDef pJump2App;
    if (((*(uint32_t *) ulAddr_App) & 0x2FFE0000) == 0x20000000)      //检查栈顶地址是否合法.
    {
        pJump2App = (pIapFun_TypeDef) *(uint32_t *) (ulAddr_App + 4);    //用户代码区第二个字为程序开始地址(复位地址)
        MSR_MSP(*(uint32_t *) ulAddr_App);                                        //初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
        pJump2App();                                                                        //跳转到APP.
    }
}

/******************* (C) COPYRIGHT 2015-2020 硬石嵌入式开发团队 *****END OF FILE****/
