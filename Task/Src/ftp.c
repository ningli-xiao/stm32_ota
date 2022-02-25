#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include "stm_flash.h"
#include "ftp.h"
#include "main.h"
#include "usart.h"
#include "iwdg.h"

#define TIME_IN   1000
#define TIME_OUT  5000
#define HEAD_LEN  96

char handle[10] = {0};//MD5值
uint8_t lteRxFlag = 0;
uint8_t lteRxBuf[LTE_USART_REC_LEN] = {0};

/*
 * 函数名：FindStrFroMem
 * 功能：从接收的数组中查找指定的字符串
 *      如果都是字符串传输可替换为strstr()函数，否则传输数据有0存在会打断
 * 输入：buf，buflen，str
 * 返回：字符串地址，null代表未找到
 */
char *FindStrFroMem(char *buf, uint16_t buflen, char *str) {
    uint8_t len = strlen(str);
    uint8_t ret;
    uint32_t temp = 0;
    if (len > buflen)
        return NULL;

    while (1) {
        ret = memcmp(buf + temp, str, len);
        if (ret == 0) {
            return (buf + temp);
        }
        temp++;
        if (buflen < temp)
            return NULL;
    }
    return NULL;
}

char *Int2String(int num, char *str)//10进制
{
    int i = 0;//指示填充str
    int j = 0;
    if (num < 0)//如果num为负数，将num变正
    {
        num = -num;
        str[i++] = '-';
    }
    //转换
    do {
        str[i++] = num % 10 + 48;//取num最低位 字符0~9的ASCII码是48~57；简单来说数字0+48=48，ASCII码对应字符'0'
        num /= 10;//去掉最低位
    } while (num); //num不为0继续循环

    str[i] = '\0';

    //确定开始调整的位置

    if (str[0] == '-')//如果有负号，负号不用调整
    {
        j = 1;//从第二位开始调整
        ++i;//由于有负号，所以交换的对称轴也要后移1位
    }
    //对称交换
    for (; j < i / 2; j++) {
        //对称交换两端的值 其实就是省下中间变量交换a+b的值：a=a+b;b=a-b;a=a-b;
        str[j] = str[j] + str[i - 1 - j];
        str[i - 1 - j] = str[j] - str[i - 1 - j];
        str[j] = str[j] - str[i - 1 - j];
    }

    return str;//返回转换后的值
}

/*
 * 函数名：ModuleOpen
 * 功能：4G模组开机
 * 输入：无
 * 返回：无
 */
char ModuleOpen(void) {
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_SET);
    HAL_Delay(600);
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_RESET);
    if (Wait_LTE_RDY(5) != 0) {
        return -1;
    }
    return 0;
}

void UART_SendData(char *pdatabuf) {
    uint16_t sizeTemp = 0;
    sizeTemp = strlen(pdatabuf);
    if (HAL_UART_Transmit(&huart1, pdatabuf, sizeTemp, 1000) != HAL_OK) {
        printf("UART_SendData ERROR\r\n");
    }
}

/*
 * 函数名：SendATCommand
 * 功能：发送AT指令
 * 输入：无
 * 返回：无
 */
char *SendATCommand(char *pCommand, char *pEcho, uint32_t outTime) {
    char *pRet = NULL;
    HAL_IWDG_Refresh(&hiwdg);//某些联网时间运行时间过长
    if (NULL != pCommand) {
        HAL_UART_DMAStop(&huart1);
        HAL_UART_Receive_DMA(&huart1, lteRxBuf, LTE_USART_REC_LEN);
        memset(lteRxBuf, 0, LTE_USART_REC_LEN);//必须先清空
        UART_SendData(pCommand);
    }
    while (--outTime)//等待指令回复
    {
        if (1 == lteRxFlag) {
            lteRxFlag = 0;
            pRet = FindStrFroMem(lteRxBuf, LTE_USART_REC_LEN, pEcho);
            if (pRet != 0) {
                return pRet;
            }//有正确回复时返回函数
        }
        HAL_Delay(1);
    }
    return pRet;
}

/*
 * 函数名：SoftReset
 * 功能：软件复位，接受到OTA指令后执行
 * 输入：无
 * 返回：无
 */
void SoftReset(void)
{
    __set_PRIMASK(1);
    HAL_NVIC_SystemReset();
}

/*
 * 函数名：MQTTClient_init
 * 功能：开机初始化等
 * 输入：无
 * 返回：
 */
int MQTTClient_init() {
    if (SendATCommand("ATE0\r\n", "OK", TIME_OUT) == 0) {
        printf("MQTT_OPENING\r\n");
        if (ModuleOpen() != 0) {
            return -1;
        }
    } else {
        printf("MQTT was OPENED\r\n");
    }
    Wait_Signal_RDY(3);
    return 0;
}

/************************************************************
** 函数名称:   ftpserver_config
** 功能描述: 配置FTP服务信息
 * 输入：OTA服务器信息
** 返 回 值:
**************************************************************/
char ftpserver_config(OtaData *ftpInfo) {
    char *rx_buf = NULL;
    char buf[128] = {0};

    rx_buf = SendATCommand("AT+QIDEACT=1\r\n", "OK", TIME_OUT);
    if (rx_buf == NULL) {
        printf("AT+QIDEACT error\r\n");
    }

    rx_buf = SendATCommand("AT+QIACT=1\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("AT+QIACT error\r\n");
        return -1;
    }

    strcpy(buf, "AT+QFTPCFG=\"account\"");
    strcat(buf, ",\"");
    strcat(buf, ftpInfo->userName);
    strcat(buf, "\",\"");
    strcat(buf, ftpInfo->password);
    strcat(buf, "\",\r\n");
    rx_buf = SendATCommand(buf, "OK", TIME_OUT);
    if (rx_buf == NULL) {
        printf("usr error\r\n");
        return -1;
    }

    printf("配置成功\r\n");
    return 0;
}

/************************************************************
** 函数名称:   ftpserver_login
** 功能描述: 登录
** 返 回 值:
**************************************************************/
char ftpserver_login(OtaData *ftpInfo) {
    char *rx_buf = NULL;
    char buf[128] = {0};
    strcpy(buf, "AT+QFTPOPEN=\"");
    strcat(buf, ftpInfo->serverAddress);
    strcat(buf, "\",");
    strcat(buf, ftpInfo->serverPort);
    strcat(buf, "\r\n");
    rx_buf = SendATCommand(buf, "+QFTPOPEN: 0,0", TIME_OUT);
    if (rx_buf == NULL) {
        printf("login error %s\r\n", lteRxBuf);
        return -1;
    } else {
        printf("注册成功\r\n");
    }
    return 0;
}

/************************************************************
** 函数名称:   ftpserver_logout
** 功能描述: 登出
** 返 回 值:
**************************************************************/
char ftpserver_logout() {
    char *rx_buf = NULL;
    rx_buf = SendATCommand("AT+QFTPCLOSE\r\n", "+QFTPCLOSE: 0,", TIME_OUT);
    if (rx_buf == NULL) {
        return -1;
    }
    return 0;
}


/************************************************************
** 函数名称: getfile_size
** 功能描述: 获取FTP服务中指定文件的大小
** 返 回 值:文件长度
**************************************************************/
uint32_t getfile_size(char *fileName) {
    char buf[128] = {0};
    char *rx_buf = NULL;

    uint32_t len = 0;

    strcpy(buf, "AT+QFTPSIZE=\"");
    strcat(buf, fileName);
    strcat(buf, "\"\r\n");

    rx_buf = SendATCommand("AT+QFTPCWD=\"/FOTA_FILES\"\r\n", "+QFTPCWD: 0,", TIME_OUT);
    if (rx_buf == NULL) {
        printf("folder error :%s", lteRxBuf);
        return 0;
    }

    rx_buf = SendATCommand(buf, "+QFTPSIZE: 0,", TIME_OUT);
    if (rx_buf == NULL) {
        printf("filesize error%s", lteRxBuf);
        return 0;
    }
    len = atoi(rx_buf + 13);
    printf("文件长度0：%d\r\n", len);
    return len;
}

/************************************************************
** 函数名称:  getfile_head
** 功能描述: 获取文件头部信息 共96字节
** 返 回 值:
**************************************************************/
char getfile_headmd5(FileData *fileInfo, char *fileName) {
    char *rx_buf = NULL;
    char *rx_handle = NULL;

    char buf[128] = {0};
    uint8_t temp;

    strcpy(buf, "AT+QFOPEN=\"");
    strcat(buf, fileName);
    strcat(buf, "\",2\r\n");//2:不存在响应error
    rx_buf = SendATCommand(buf, "OK", TIME_IN);//+QFOPEN: <filehandle> OK
    if (rx_buf != NULL) {
        printf("open ok:%s\r\n", lteRxBuf);
    } else {
        printf("open error:%s\r\n", lteRxBuf);
        return -1;
    }

    rx_handle = FindStrFroMem((char *) &lteRxBuf[0], 200, "+QFOPEN:");
    if (rx_handle != NULL) {
        rx_handle += 9;
        memcpy(fileInfo->handle, rx_handle, (uint8_t) (rx_buf - rx_handle - 4));//减去两个0D0A
        printf("get handle right:%s\r\n", handle);
    } else {
        printf("get handle error:%s\r\n", lteRxBuf);
        return -1;
    }

    memset(buf, 0, 128);
    strcpy(buf, "AT+QFREAD=");
    strcat(buf, fileInfo->handle);
    strcat(buf, ",96\r\n");
    rx_buf = SendATCommand(buf, "OK", TIME_IN);

    if (rx_buf != 0) {
        rx_buf = FindStrFroMem((char *) &lteRxBuf[0], LTE_USART_REC_LEN, "CONNECT");
        if (rx_buf != 0) {
            rx_buf += 9;
            printf("读取MD5 ok:%s\r\n", lteRxBuf);
            memcpy(fileInfo->md5, rx_buf + 80, 16);
        } else {
            printf("MD5 no connect:%s\r\n", lteRxBuf);
            return -1;
        }
    } else {
        printf("读取MD5 error:%s\r\n", lteRxBuf);
        return -1;
    }
    return 0;
}

/************************************************************
** 函数名称:  deletefile
** 功能描述: 删除UFS里存储的文件
** 返 回 值: 结果
**************************************************************/
char deletefile() {
    if (0 != SendATCommand("AT+QFDEL=\"*\"\r\n", "OK", TIME_OUT)) {
        printf("删除成功%s\r\n", lteRxBuf);
        return 0;
    }
    return -1;
}

/************************************************************
** 函数名称:  wait_download
** 功能描述: 响应OK后等待下载全部完成
** 返 回 值:
**************************************************************/
char wait_download() {
    if (SendATCommand(NULL, "+QFTPGET: 0", TIME_OUT * 3) == 0) {
        return -1;
    }
    return 0;
}

/************************************************************
** 函数名称:   downloadfile
** 功能描述: 从FTP下载文件，每次下载1024字节
** 返 回 值:
**************************************************************/
char downloadfile(OtaData *otaInfo) {
    char *rx_buf = NULL;
    char buf[128] = {0};

    strcpy(buf, "AT+QFTPGET=\"");
    strcat(buf, otaInfo->fileName);
    strcat(buf, "\",");
    strcat(buf, "\"UFS:app.bin\"\r\n");

    rx_buf = SendATCommand(buf, "OK", TIME_OUT);
    if (rx_buf != NULL) {
        printf("下载失败%s\r\n", lteRxBuf);
        return -1;
    }

    if (wait_download() != 0) {
        printf("下载失败%s\r\n", lteRxBuf);
        return -1;
    }

    return 0;
}

/************************************************************
** 函数名称:  writefile
** 功能描述: 从4G模组读取并写到FLASH里，每次下载1024字节
 * 注意要减去96的头文件长度
** 返 回 值:
**************************************************************/
char writefile(uint32_t len,char* FileHandle) {
    static uint16_t count = 0;

    char *rx_buf = NULL;
    char str_len[5]={0};
    char buf[128] = {0};

    uint32_t time = 0;
    uint32_t rest_flag = 0;

    time = len / DOWNLOADLEN;
    rest_flag = len % DOWNLOADLEN;//是否有剩余包

    if (count < time) {
        memset(buf, 0, 128);
        memset(str_len, 0, 5);
        strcpy(buf, "AT+QFSEEK=");
        strcat(buf, FileHandle);
        strcat(buf, ",");
        sprintf(str_len,"%d",HEAD_LEN + count * DOWNLOADLEN);
        strcat(buf, str_len);
        strcat(buf, ",0\r\n");//每次从文件最初位置开始移动

        rx_buf = SendATCommand(buf, str_len, TIME_IN);
        if (rx_buf != 0) {
            printf("seek right:%s\r\n", lteRxBuf);
        } else {
            printf("seek error:%s\r\n", lteRxBuf);
        }

        memset(buf, 0, 128);
        memset(str_len, 0, 5);
        strcpy(buf, "AT+QFREAD=");
        strcat(buf, FileHandle);
        strcat(buf, ",");
        sprintf(str_len,"%d",DOWNLOADLEN);//读取1024长度
        strcat(buf, str_len);
        strcat(buf, "\r\n");

        rx_buf = SendATCommand(buf, "OK", TIME_OUT);
        if (rx_buf != 0) {
            rx_buf = FindStrFroMem((char *) lteRxBuf, LTE_USART_REC_LEN, "CONNECT");
            if (rx_buf != 0) {
                rx_buf += 9;
                STMFLASH_Write(FLASH_AppAddress + count * DOWNLOADLEN, (uint16_t *) rx_buf, DOWNLOADLEN / 2);
            } else {
                printf("find not connect\r\n");
                return -1;
            }
        } else {
            printf("读取文件error:%s\r\n", lteRxBuf);
            return -1;
        }
    }

    if (count == time && rest_flag != 0)//有剩余包且到达指定位置
    {
        rest_flag = 0;
        memset(buf, 0, 128);
        memset(str_len, 0, 5);
        strcpy(buf, "AT+QFSEEK=");
        strcat(buf, FileHandle);
        strcat(buf, ",");
        sprintf(str_len,"%d",HEAD_LEN + count * DOWNLOADLEN);
        strcat(buf, str_len);
        strcat(buf, ",0\r\n");//每次从文件最初位置开始移动

        rx_buf = SendATCommand(buf, Int2String(HEAD_LEN + count * DOWNLOADLEN, str_len), TIME_IN);

        if (rx_buf != 0) {
            printf("seek right:%s\r\n", lteRxBuf);
        } else {
            printf("seek error:%s\r\n", lteRxBuf);
            //return -1;//seek的OK一直都是error
        }

        memset(buf, 0, 128);
        memset(str_len, 0, 5);
        strcpy(buf, "AT+QFREAD=");
        strcat(buf, FileHandle);
        strcat(buf, ",");
        sprintf(str_len,"%d",DOWNLOADLEN);//读取1024长度
        strcat(buf, str_len);
        strcat(buf, "\r\n");

        rx_buf = SendATCommand(buf, "OK", TIME_OUT);

        if (rx_buf != 0) {
            rx_buf = FindStrFroMem((char *) lteRxBuf, LTE_USART_REC_LEN, "CONNECT");
            if (rx_buf != 0) {
                rx_buf += 9;
                STMFLASH_Write(FLASH_AppAddress + count * DOWNLOADLEN, (uint16_t *) rx_buf, DOWNLOADLEN / 2);
            } else {
                printf("find not connect%s\r\n", lteRxBuf);
                return -1;
            }
        } else {
            printf("读取文件error:%s\r\n", lteRxBuf);
            return -1;
        }

        memset(buf, 0, 128);
        strcpy(buf, "AT+QFCLOSE=");
        strcat(buf, FileHandle);
        strcat(buf, "\r\n");

        if (0 != SendATCommand(buf, "OK", TIME_IN)) {
            printf("close file right:%s\r\n", lteRxBuf);
        } else {
            printf("close file error\r\n");
            return -1;
        }
    }
    count++;
    return count;
}

int Wait_LTE_RDY(uint8_t time) {
    while (--time) {
        if (SendATCommand(NULL, "RDY", TIME_IN) == 0) {
            printf(" No  RDY \r\n");
        } else {
            printf(" RDY Come\r\n");
            return 0;
        }
        HAL_Delay(1000);
    }
    printf(" Wait_LTE_RDY Timeout\r\n");
    return -1;
}

//判断信号质量
int LTE_Signal_Quality(void) {
    char *pRet = 0;
    pRet = SendATCommand("AT+CSQ\r\n", "OK", TIME_IN);
    if (pRet == 0) {
        return -1;
    }
    printf("signal_value:%s\r\n", lteRxBuf);
    if (strstr((char *) pRet, "99,99") != 0) {
        return -1;
    }
    return 0;
}

//下载文件并
char downloadAndWrite() {
    mcuFileData.app_size = getfile_size(mcuOtaData.fileName)-96;
    if(mcuFileData.app_size==0){
        printf("app_size error\r\n");
        return -1;
    }
    deletefile();
    if (downloadfile(&mcuOtaData) != 0) {
        printf("download error\r\n");
        return -1;
    }
    if (writefile(mcuFileData.app_size - 96, mcuFileData.handle) != 0) {
        printf("write error\r\n");
        return -1;
    }
    return 0;
}

int Wait_Signal_RDY(uint8_t time) {
    while (--time) {
        if (LTE_Signal_Quality() != 0) {
            printf(" No  Signal \r\n");
        } else {
            printf(" Signal OK\r\n");
            return 0;
        }
        HAL_Delay(500);
    }
    printf(" Wait_Signal_RDY Timeout\r\n");
    return -1;
}

















