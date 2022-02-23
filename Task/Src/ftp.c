#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include "stm_flash.h"
#include "ftp.h"
#include "main.h"
#include "usart.h"

#define USERNAME "bicycleftp"
#define PASSWORD "34tjYXBw"

#define FTP_SERVER "123.7.182.34"
#define SERVER_PORT 30021

#define TIME_IN   200
#define TIME_OUT  1000
#define HEAD_LEN  96

uint32_t app_size = 0;//bin文件大小
uint16_t app_version = 0;//版本号
uint8_t updata_flag = 0;//升级标志
uint8_t app_md5[17] = {0};//MD5值
char handle[10] = {0};//MD5值
uint8_t Lte_RX_ok = 0;
uint8_t Lte_RX_BUF[LTE_USART_REC_LEN] = {0};
uint8_t error_count = 0;
uint16_t wLteUsartReceiveSize = 0;

/*
 * 函数名：FindStrFroMem
 * 功能：从接收的数组中查找指定的字符串
 *      如果都是字符串传输可替换为strstr()函数，否则传输数据有0存在会打断
 * 输入：buf，buflen，str
 * 返回：字符串地址，null代表未找到
 */
char* FindStrFroMem(char *buf, uint16_t buflen, char *str)
{
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
static void ModuleOpen(void) {
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_SET);
    HAL_Delay(1500);
    HAL_GPIO_WritePin(ONOFF_EC200_GPIO_Port, ONOFF_EC200_Pin, GPIO_PIN_RESET);
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
    int i = 0;
    if (NULL != pCommand) {
        memset(Lte_RX_BUF, 0, LTE_USART_REC_LEN);//必须先清空
        UART_SendData((uint8_t *) pCommand);
    }
    while (--outTime)//等待指令回复
    {
        if (1 == Lte_RX_ok) {
            Lte_RX_ok = 0;
            pRet = FindStrFroMem(Lte_RX_BUF, LTE_USART_REC_LEN, pEcho);
            if (pRet != 0) { return pRet; }//有正确回复时返回函数
        }
        HAL_Delay(1);
    }
    return pRet;
}

/************************************************************
** 函数名称:   ftpserver_config
** 功能描述: 配置FTP服务信息
** 返 回 值:
** 作     者:qifei
** 日     期:2020.8.06
**************************************************************/
char ftpserver_config() {
    char *rx_buf = NULL;
    rx_buf = SendATCommand("ATE0\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("ATE0 error\r\n");
        return -1;
    }

    rx_buf = SendATCommand("AT+QIDEACT=1\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("AT+QIDEACT error\r\n");
    }

    rx_buf = SendATCommand("AT+QIACT=1\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("配置失败2\r\n");
        return -1;
    }

    rx_buf = SendATCommand("AT+QFTPCFG=\"contextid\",1\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("配置失败3\r\n");
        return -1;
    }

    rx_buf = SendATCommand("AT+QFTPCFG=\"account\",\"bicycleftp\",\"34tjYXBw\"\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("配置失败4\r\n");
        return -1;
    }

    rx_buf = SendATCommand("AT+QFTPCFG=\"filetype\",0\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("配置失败5\r\n");
        return -1;
    }
    rx_buf = SendATCommand("AT+QFTPCFG=\"transmode\",1\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("配置失败6\r\n");
        return -1;
    }

    rx_buf = SendATCommand("AT+QFTPCFG=\"rsptimeout\",180\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("配置失败7\r\n");

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
char ftpserver_login() {
    char *rx_buf = NULL;
    rx_buf = SendATCommand("AT+QFTPOPEN=\"123.7.182.34\",30021\r\n", "+QFTPOPEN: 0,", TIME_OUT);
    if (rx_buf == NULL) {
        printf("注册失败\r\n");
        printf("%s", Lte_RX_BUF);
        return -1;
    } else {
        printf("注册成功\r\n");
        printf("%s", Lte_RX_BUF);
    }
    return 0;
}

/************************************************************
** 函数名称:   ftpserver_logout
** 功能描述: 登出
** 返 回 值:
** 作     者:qifei
** 日     期:2020.8.06
**************************************************************/
void ftpserver_logout() {
    char *rx_buf = NULL;

    rx_buf = SendATCommand("AT+QFTPCLOSE\r\n", "+QFTPCLOSE: 0,", TIME_OUT);
    if (rx_buf == NULL) {
        return;
    }
}

/************************************************************
** 函数名称:   getfile_list
** 功能描述: 获取文件列表
** 返 回 值:
**************************************************************/
uint32_t getfile_list() {

}

/************************************************************
** 函数名称:   getfile_size
** 功能描述: 获取FTP服务中指定文件的大小
** 返 回 值:
**************************************************************/
uint32_t getfile_size() {
    char file_size[100] = "AT+QFTPSIZE=\"G30L_4G_V";
    char *rx_buf = NULL;
    uint32_t len = 0;
    uint8_t str_len[5] = {0};
    uint8_t atlen = strlen(file_size);

    memset(file_size + atlen, 0, 20);
    strcat(file_size, Int2String(app_version, str_len));
    strcat(file_size, ".bin\"");
    strcat(file_size, "\r\n");

    rx_buf = SendATCommand("AT+QFTPCWD=\"/G30L\"\r\n", "+QFTPCWD: 0,", TIME_OUT);
    if (rx_buf == NULL) {
        printf("%s", Lte_RX_BUF);
        printf("文件长度0\r\n");
        return 0;
    }
    rx_buf = SendATCommand(file_size, "+QFTPSIZE: 0,", TIME_OUT);
    if (rx_buf == NULL) {
        printf("%s", Lte_RX_BUF);
        printf("文件长度1\r\n");
        return 0;
    }
    len = atoi(rx_buf + 13);
    printf("文件长度0：%d\r\n", len);
    return len;
}

/************************************************************
** 函数名称:   getfile_head
** 功能描述: 获取文件头部信息 共96字节
** 返 回 值:
**************************************************************/
char getfile_headmd5(uint8_t *md5) {
    char *rx_buf = NULL;
    char *rx_handle = NULL;
    char str_len[5] = {0};
    char atcmd[100] = {0};
    uint8_t atlen = 0;
    uint8_t temp;
    rx_buf = SendATCommand("AT+QFOPEN=\"G30L.bin\",0\r\n", "OK", TIME_IN);
    if (rx_buf != NULL) {
        printf("打开成功:%s\r\n", Lte_RX_BUF);

    } else {
        printf("打开失败:%s\r\n", Lte_RX_BUF);
        return -1;
    }
    rx_handle = FindStrFroMem((char *) &Lte_RX_BUF[0], 200, "+QFOPEN:");

    if (rx_handle != NULL) {
        rx_handle += 9;
        memcpy(&handle[0], rx_handle, (uint8_t) (rx_buf - rx_handle - 4));//减去两个0D0A
        printf("get handle right:%s\r\n", handle);
    } else {
        printf("get handle error:%s\r\n", Lte_RX_BUF);
        return -1;
    }

    memset(atcmd, 0, 20);
    memcpy(atcmd, "AT+QFREAD=", 10);
    atlen = strlen(atcmd);
    memset(atcmd + atlen, 0, 20);
    strcat(atcmd, handle);
    strcat(atcmd, ",96\r\n");
    rx_buf = SendATCommand(atcmd, "OK", TIME_IN);

    if (rx_buf != 0) {
        rx_buf = FindStrFroMem((char *) &Lte_RX_BUF[0], LTE_USART_REC_LEN, "CONNECT");
        if (rx_buf != 0) {
            rx_buf += 9;
            printf("读取MD5 ok:%s\r\n", Lte_RX_BUF);
            memcpy(md5, rx_buf + 80, 16);
        } else {
            printf("MD5 no connect:%s\r\n", Lte_RX_BUF);
            return -1;
        }

    } else {
        printf("读取MD5 error:%s\r\n", Lte_RX_BUF);
        return -1;
    }
    return 0;
}

/************************************************************
** 函数名称:     deletefile
** 功能描述: 删除UFS里存储的文件
** 返 回 值: 结果
**************************************************************/
char deletefile() {
    if (0 != SendATCommand("AT+QFDEL=\"*\"\r\n", "OK", TIME_OUT)) {
        printf("删除成功%s\r\n", Lte_RX_BUF);
        return 0;
    }
    return -1;
}

/************************************************************
** 函数名称:   downloadfile
** 功能描述: 从FTP下载文件，每次下载1024字节
** 返 回 值:
**************************************************************/
char downloadfile() {
    char error_count = 0;
    char *rx_buf = NULL;
    char str_len[5] = {0};
    char atcmd[100] = "AT+QFTPGET=\"G30L_4G_V";
    char handle[10] = {0};
    uint8_t temp = 0;
    uint8_t atlen = strlen(atcmd);

    memset(atcmd + atlen, 0, 30);
    strcat(atcmd, Int2String(app_version, str_len));
    strcat(atcmd, ".bin\",");
    strcat(atcmd, "\"UFS:G30L.bin\"\r\n");

    rx_buf = SendATCommand(atcmd, "OK", TIME_OUT);
    if (rx_buf != NULL) {
        printf("下载成功%s\r\n", Lte_RX_BUF);
        return 0;
    } else {
        printf("下载失败%s\r\n", Lte_RX_BUF);
        return -1;
    }
}

/************************************************************
** 函数名称:    writefile
** 功能描述:从4G模组读取并写到SPIFLASH里，每次下载1024字节
** 返 回 值:
** 作     者:lixiaoning
** 日     期:2021.6.25
**************************************************************/
char writefile(uint32_t len) {
    static uint16_t count = 0;
    char *rx_buf = NULL;
    char str_len[5] = {0};
    char atcmd[100] = {0};
    uint8_t atlen = 0;
    uint32_t time = 0;
    uint32_t rest_flag = 0;

    time = len / DOWNLOADLEN;
    rest_flag = len % DOWNLOADLEN;//是否有剩余包

    if (count < time) {
        memset(atcmd, 0, 20);
        memcpy(atcmd, "AT+QFSEEK=", 10);
        atlen = strlen(atcmd);
        memset(atcmd + atlen, 0, 20);

        strcat(atcmd, handle);
        strcat(atcmd, ",");
        strcat(atcmd, Int2String(HEAD_LEN + count * DOWNLOADLEN, str_len));
        strcat(atcmd, "\r\n");

        rx_buf = SendATCommand(atcmd, Int2String(HEAD_LEN + count * DOWNLOADLEN, str_len), TIME_IN);

        if (rx_buf != 0) {
            printf("seek right:%s\r\n", Lte_RX_BUF);
        } else {
            printf("seek error:%s\r\n", Lte_RX_BUF);
        }

        memset(atcmd, 0, 20);
        memcpy(atcmd, "AT+QFREAD=", 10);
        atlen = strlen(atcmd);
        memset(atcmd + atlen, 0, 20);
        strcat(atcmd, handle);

        strcat(atcmd, ",");
        strcat(atcmd, Int2String(DOWNLOADLEN, str_len));
        strcat(atcmd, "\r\n");

        rx_buf = SendATCommand(atcmd, "OK", TIME_OUT);
        if (rx_buf != 0) {
            rx_buf = FindStrFroMem((char *) Lte_RX_BUF, LTE_USART_REC_LEN, "CONNECT");
            if (rx_buf != 0) {
                rx_buf += 9;
                STMFLASH_Write(FLASH_AppAddress + count * DOWNLOADLEN, (uint16_t *)rx_buf, DOWNLOADLEN/2);
            } else {
                printf("find not connect\r\n");
                return -1;
            }
        } else {
            printf("读取文件error:%s\r\n", Lte_RX_BUF);
            return -1;
        }
    }

    if (count == time && rest_flag != 0)//有剩余包且到达指定位置
    {
        rest_flag = 0;
        memset(atcmd, 0, 20);
        memcpy(atcmd, "AT+QFSEEK=", 10);
        atlen = strlen(atcmd);
        memset(atcmd + atlen, 0, 20);

        strcat(atcmd, handle);
        strcat(atcmd, ",");
        strcat(atcmd, Int2String(HEAD_LEN + count * DOWNLOADLEN, str_len));
        strcat(atcmd, "\r\n");

        rx_buf = SendATCommand(atcmd, Int2String(HEAD_LEN + count * DOWNLOADLEN, str_len), TIME_IN);

        if (rx_buf != 0) {
            printf("seek right:%s\r\n", Lte_RX_BUF);
        } else {
            printf("seek error:%s\r\n", Lte_RX_BUF);
            //return -1;//seek的OK一直都是error
        }

        memset(atcmd, 0, 20);
        memcpy(atcmd, "AT+QFREAD=", 10);
        atlen = strlen(atcmd);
        memset(atcmd + atlen, 0, 20);
        strcat(atcmd, handle);

        strcat(atcmd, ",");
        strcat(atcmd, Int2String(len % DOWNLOADLEN, str_len));
        strcat(atcmd, "\r\n");

        rx_buf = SendATCommand(atcmd, "OK", TIME_OUT);

        if (rx_buf != 0) {
            rx_buf = FindStrFroMem((char *) Lte_RX_BUF, LTE_USART_REC_LEN, "CONNECT");
            if (rx_buf != 0) {
                rx_buf += 9;
                STMFLASH_Write(FLASH_AppAddress + count * DOWNLOADLEN, (uint16_t *)rx_buf, DOWNLOADLEN/2);
            } else {
                printf("find not connect%s\r\n", Lte_RX_BUF);
                return -1;
            }
        } else {
            printf("读取文件error:%s\r\n", Lte_RX_BUF);
            return -1;
        }

        memset(atcmd, 0, 20);
        memcpy(atcmd, "AT+QFCLOSE=", 11);
        atlen = strlen(atcmd);
        memset(atcmd + atlen, 0, 20);
        strcat(atcmd, handle);
        strcat(atcmd, "\r\n");
        if (0 != SendATCommand(atcmd, "OK", TIME_IN)) {
            printf("关闭结果:%s\r\n", Lte_RX_BUF);
        } else {
            printf("关闭error\r\n");
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
    printf("signal_value:%s\r\n", Lte_RX_BUF);
    if (strstr((char *) pRet, "99,99") != 0) {
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

















