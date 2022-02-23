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

uint32_t app_size = 0;//bin�ļ���С
uint16_t app_version = 0;//�汾��
uint8_t updata_flag = 0;//������־
uint8_t app_md5[17] = {0};//MD5ֵ
char handle[10] = {0};//MD5ֵ
uint8_t Lte_RX_ok = 0;
uint8_t Lte_RX_BUF[LTE_USART_REC_LEN] = {0};
uint8_t error_count = 0;
uint16_t wLteUsartReceiveSize = 0;

/*
 * ��������FindStrFroMem
 * ���ܣ��ӽ��յ������в���ָ�����ַ���
 *      ��������ַ���������滻Ϊstrstr()������������������0���ڻ���
 * ���룺buf��buflen��str
 * ���أ��ַ�����ַ��null����δ�ҵ�
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

char *Int2String(int num, char *str)//10����
{
    int i = 0;//ָʾ���str
    int j = 0;
    if (num < 0)//���numΪ��������num����
    {
        num = -num;
        str[i++] = '-';
    }
    //ת��
    do {
        str[i++] = num % 10 + 48;//ȡnum���λ �ַ�0~9��ASCII����48~57������˵����0+48=48��ASCII���Ӧ�ַ�'0'
        num /= 10;//ȥ�����λ
    } while (num); //num��Ϊ0����ѭ��

    str[i] = '\0';

    //ȷ����ʼ������λ��

    if (str[0] == '-')//����и��ţ����Ų��õ���
    {
        j = 1;//�ӵڶ�λ��ʼ����
        ++i;//�����и��ţ����Խ����ĶԳ���ҲҪ����1λ
    }
    //�Գƽ���
    for (; j < i / 2; j++) {
        //�Գƽ������˵�ֵ ��ʵ����ʡ���м��������a+b��ֵ��a=a+b;b=a-b;a=a-b;
        str[j] = str[j] + str[i - 1 - j];
        str[i - 1 - j] = str[j] - str[i - 1 - j];
        str[j] = str[j] - str[i - 1 - j];
    }

    return str;//����ת�����ֵ
}

/*
 * ��������ModuleOpen
 * ���ܣ�4Gģ�鿪��
 * ���룺��
 * ���أ���
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
 * ��������SendATCommand
 * ���ܣ�����ATָ��
 * ���룺��
 * ���أ���
 */
char *SendATCommand(char *pCommand, char *pEcho, uint32_t outTime) {
    char *pRet = NULL;
    int i = 0;
    if (NULL != pCommand) {
        memset(Lte_RX_BUF, 0, LTE_USART_REC_LEN);//���������
        UART_SendData((uint8_t *) pCommand);
    }
    while (--outTime)//�ȴ�ָ��ظ�
    {
        if (1 == Lte_RX_ok) {
            Lte_RX_ok = 0;
            pRet = FindStrFroMem(Lte_RX_BUF, LTE_USART_REC_LEN, pEcho);
            if (pRet != 0) { return pRet; }//����ȷ�ظ�ʱ���غ���
        }
        HAL_Delay(1);
    }
    return pRet;
}

/************************************************************
** ��������:   ftpserver_config
** ��������: ����FTP������Ϣ
** �� �� ֵ:
** ��     ��:qifei
** ��     ��:2020.8.06
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
        printf("����ʧ��2\r\n");
        return -1;
    }

    rx_buf = SendATCommand("AT+QFTPCFG=\"contextid\",1\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("����ʧ��3\r\n");
        return -1;
    }

    rx_buf = SendATCommand("AT+QFTPCFG=\"account\",\"bicycleftp\",\"34tjYXBw\"\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("����ʧ��4\r\n");
        return -1;
    }

    rx_buf = SendATCommand("AT+QFTPCFG=\"filetype\",0\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("����ʧ��5\r\n");
        return -1;
    }
    rx_buf = SendATCommand("AT+QFTPCFG=\"transmode\",1\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("����ʧ��6\r\n");
        return -1;
    }

    rx_buf = SendATCommand("AT+QFTPCFG=\"rsptimeout\",180\r\n", "OK", TIME_IN);
    if (rx_buf == NULL) {
        printf("����ʧ��7\r\n");

        return -1;
    }
    printf("���óɹ�\r\n");
    return 0;

}

/************************************************************
** ��������:   ftpserver_login
** ��������: ��¼
** �� �� ֵ:
**************************************************************/
char ftpserver_login() {
    char *rx_buf = NULL;
    rx_buf = SendATCommand("AT+QFTPOPEN=\"123.7.182.34\",30021\r\n", "+QFTPOPEN: 0,", TIME_OUT);
    if (rx_buf == NULL) {
        printf("ע��ʧ��\r\n");
        printf("%s", Lte_RX_BUF);
        return -1;
    } else {
        printf("ע��ɹ�\r\n");
        printf("%s", Lte_RX_BUF);
    }
    return 0;
}

/************************************************************
** ��������:   ftpserver_logout
** ��������: �ǳ�
** �� �� ֵ:
** ��     ��:qifei
** ��     ��:2020.8.06
**************************************************************/
void ftpserver_logout() {
    char *rx_buf = NULL;

    rx_buf = SendATCommand("AT+QFTPCLOSE\r\n", "+QFTPCLOSE: 0,", TIME_OUT);
    if (rx_buf == NULL) {
        return;
    }
}

/************************************************************
** ��������:   getfile_list
** ��������: ��ȡ�ļ��б�
** �� �� ֵ:
**************************************************************/
uint32_t getfile_list() {

}

/************************************************************
** ��������:   getfile_size
** ��������: ��ȡFTP������ָ���ļ��Ĵ�С
** �� �� ֵ:
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
        printf("�ļ�����0\r\n");
        return 0;
    }
    rx_buf = SendATCommand(file_size, "+QFTPSIZE: 0,", TIME_OUT);
    if (rx_buf == NULL) {
        printf("%s", Lte_RX_BUF);
        printf("�ļ�����1\r\n");
        return 0;
    }
    len = atoi(rx_buf + 13);
    printf("�ļ�����0��%d\r\n", len);
    return len;
}

/************************************************************
** ��������:   getfile_head
** ��������: ��ȡ�ļ�ͷ����Ϣ ��96�ֽ�
** �� �� ֵ:
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
        printf("�򿪳ɹ�:%s\r\n", Lte_RX_BUF);

    } else {
        printf("��ʧ��:%s\r\n", Lte_RX_BUF);
        return -1;
    }
    rx_handle = FindStrFroMem((char *) &Lte_RX_BUF[0], 200, "+QFOPEN:");

    if (rx_handle != NULL) {
        rx_handle += 9;
        memcpy(&handle[0], rx_handle, (uint8_t) (rx_buf - rx_handle - 4));//��ȥ����0D0A
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
            printf("��ȡMD5 ok:%s\r\n", Lte_RX_BUF);
            memcpy(md5, rx_buf + 80, 16);
        } else {
            printf("MD5 no connect:%s\r\n", Lte_RX_BUF);
            return -1;
        }

    } else {
        printf("��ȡMD5 error:%s\r\n", Lte_RX_BUF);
        return -1;
    }
    return 0;
}

/************************************************************
** ��������:     deletefile
** ��������: ɾ��UFS��洢���ļ�
** �� �� ֵ: ���
**************************************************************/
char deletefile() {
    if (0 != SendATCommand("AT+QFDEL=\"*\"\r\n", "OK", TIME_OUT)) {
        printf("ɾ���ɹ�%s\r\n", Lte_RX_BUF);
        return 0;
    }
    return -1;
}

/************************************************************
** ��������:   downloadfile
** ��������: ��FTP�����ļ���ÿ������1024�ֽ�
** �� �� ֵ:
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
        printf("���سɹ�%s\r\n", Lte_RX_BUF);
        return 0;
    } else {
        printf("����ʧ��%s\r\n", Lte_RX_BUF);
        return -1;
    }
}

/************************************************************
** ��������:    writefile
** ��������:��4Gģ���ȡ��д��SPIFLASH�ÿ������1024�ֽ�
** �� �� ֵ:
** ��     ��:lixiaoning
** ��     ��:2021.6.25
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
    rest_flag = len % DOWNLOADLEN;//�Ƿ���ʣ���

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
            printf("��ȡ�ļ�error:%s\r\n", Lte_RX_BUF);
            return -1;
        }
    }

    if (count == time && rest_flag != 0)//��ʣ����ҵ���ָ��λ��
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
            //return -1;//seek��OKһֱ����error
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
            printf("��ȡ�ļ�error:%s\r\n", Lte_RX_BUF);
            return -1;
        }

        memset(atcmd, 0, 20);
        memcpy(atcmd, "AT+QFCLOSE=", 11);
        atlen = strlen(atcmd);
        memset(atcmd + atlen, 0, 20);
        strcat(atcmd, handle);
        strcat(atcmd, "\r\n");
        if (0 != SendATCommand(atcmd, "OK", TIME_IN)) {
            printf("�رս��:%s\r\n", Lte_RX_BUF);
        } else {
            printf("�ر�error\r\n");
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

//�ж��ź�����
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

















