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

char handle[10] = {0};//MD5ֵ
uint8_t lteRxFlag = 0;
uint8_t lteRxBuf[LTE_USART_REC_LEN] = {0};

/*
 * ��������FindStrFroMem
 * ���ܣ��ӽ��յ������в���ָ�����ַ���
 *      ��������ַ���������滻Ϊstrstr()������������������0���ڻ���
 * ���룺buf��buflen��str
 * ���أ��ַ�����ַ��null����δ�ҵ�
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
 * ��������SendATCommand
 * ���ܣ�����ATָ��
 * ���룺��
 * ���أ���
 */
char *SendATCommand(char *pCommand, char *pEcho, uint32_t outTime) {
    char *pRet = NULL;
    HAL_IWDG_Refresh(&hiwdg);//ĳЩ����ʱ������ʱ�����
    if (NULL != pCommand) {
        HAL_UART_DMAStop(&huart1);
        HAL_UART_Receive_DMA(&huart1, lteRxBuf, LTE_USART_REC_LEN);
        memset(lteRxBuf, 0, LTE_USART_REC_LEN);//���������
        UART_SendData(pCommand);
    }
    while (--outTime)//�ȴ�ָ��ظ�
    {
        if (1 == lteRxFlag) {
            lteRxFlag = 0;
            pRet = FindStrFroMem(lteRxBuf, LTE_USART_REC_LEN, pEcho);
            if (pRet != 0) {
                return pRet;
            }//����ȷ�ظ�ʱ���غ���
        }
        HAL_Delay(1);
    }
    return pRet;
}

/*
 * ��������SoftReset
 * ���ܣ������λ�����ܵ�OTAָ���ִ��
 * ���룺��
 * ���أ���
 */
void SoftReset(void)
{
    __set_PRIMASK(1);
    HAL_NVIC_SystemReset();
}

/*
 * ��������MQTTClient_init
 * ���ܣ�������ʼ����
 * ���룺��
 * ���أ�
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
** ��������:   ftpserver_config
** ��������: ����FTP������Ϣ
 * ���룺OTA��������Ϣ
** �� �� ֵ:
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

    printf("���óɹ�\r\n");
    return 0;
}

/************************************************************
** ��������:   ftpserver_login
** ��������: ��¼
** �� �� ֵ:
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
        printf("ע��ɹ�\r\n");
    }
    return 0;
}

/************************************************************
** ��������:   ftpserver_logout
** ��������: �ǳ�
** �� �� ֵ:
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
** ��������: getfile_size
** ��������: ��ȡFTP������ָ���ļ��Ĵ�С
** �� �� ֵ:�ļ�����
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
    printf("�ļ�����0��%d\r\n", len);
    return len;
}

/************************************************************
** ��������:  getfile_head
** ��������: ��ȡ�ļ�ͷ����Ϣ ��96�ֽ�
** �� �� ֵ:
**************************************************************/
char getfile_headmd5(FileData *fileInfo, char *fileName) {
    char *rx_buf = NULL;
    char *rx_handle = NULL;

    char buf[128] = {0};
    uint8_t temp;

    strcpy(buf, "AT+QFOPEN=\"");
    strcat(buf, fileName);
    strcat(buf, "\",2\r\n");//2:��������Ӧerror
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
        memcpy(fileInfo->handle, rx_handle, (uint8_t) (rx_buf - rx_handle - 4));//��ȥ����0D0A
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
            printf("��ȡMD5 ok:%s\r\n", lteRxBuf);
            memcpy(fileInfo->md5, rx_buf + 80, 16);
        } else {
            printf("MD5 no connect:%s\r\n", lteRxBuf);
            return -1;
        }
    } else {
        printf("��ȡMD5 error:%s\r\n", lteRxBuf);
        return -1;
    }
    return 0;
}

/************************************************************
** ��������:  deletefile
** ��������: ɾ��UFS��洢���ļ�
** �� �� ֵ: ���
**************************************************************/
char deletefile() {
    if (0 != SendATCommand("AT+QFDEL=\"*\"\r\n", "OK", TIME_OUT)) {
        printf("ɾ���ɹ�%s\r\n", lteRxBuf);
        return 0;
    }
    return -1;
}

/************************************************************
** ��������:  wait_download
** ��������: ��ӦOK��ȴ�����ȫ�����
** �� �� ֵ:
**************************************************************/
char wait_download() {
    if (SendATCommand(NULL, "+QFTPGET: 0", TIME_OUT * 3) == 0) {
        return -1;
    }
    return 0;
}

/************************************************************
** ��������:   downloadfile
** ��������: ��FTP�����ļ���ÿ������1024�ֽ�
** �� �� ֵ:
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
        printf("����ʧ��%s\r\n", lteRxBuf);
        return -1;
    }

    if (wait_download() != 0) {
        printf("����ʧ��%s\r\n", lteRxBuf);
        return -1;
    }

    return 0;
}

/************************************************************
** ��������:  writefile
** ��������: ��4Gģ���ȡ��д��FLASH�ÿ������1024�ֽ�
 * ע��Ҫ��ȥ96��ͷ�ļ�����
** �� �� ֵ:
**************************************************************/
char writefile(uint32_t len,char* FileHandle) {
    static uint16_t count = 0;

    char *rx_buf = NULL;
    char str_len[5]={0};
    char buf[128] = {0};

    uint32_t time = 0;
    uint32_t rest_flag = 0;

    time = len / DOWNLOADLEN;
    rest_flag = len % DOWNLOADLEN;//�Ƿ���ʣ���

    if (count < time) {
        memset(buf, 0, 128);
        memset(str_len, 0, 5);
        strcpy(buf, "AT+QFSEEK=");
        strcat(buf, FileHandle);
        strcat(buf, ",");
        sprintf(str_len,"%d",HEAD_LEN + count * DOWNLOADLEN);
        strcat(buf, str_len);
        strcat(buf, ",0\r\n");//ÿ�δ��ļ����λ�ÿ�ʼ�ƶ�

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
        sprintf(str_len,"%d",DOWNLOADLEN);//��ȡ1024����
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
            printf("��ȡ�ļ�error:%s\r\n", lteRxBuf);
            return -1;
        }
    }

    if (count == time && rest_flag != 0)//��ʣ����ҵ���ָ��λ��
    {
        rest_flag = 0;
        memset(buf, 0, 128);
        memset(str_len, 0, 5);
        strcpy(buf, "AT+QFSEEK=");
        strcat(buf, FileHandle);
        strcat(buf, ",");
        sprintf(str_len,"%d",HEAD_LEN + count * DOWNLOADLEN);
        strcat(buf, str_len);
        strcat(buf, ",0\r\n");//ÿ�δ��ļ����λ�ÿ�ʼ�ƶ�

        rx_buf = SendATCommand(buf, Int2String(HEAD_LEN + count * DOWNLOADLEN, str_len), TIME_IN);

        if (rx_buf != 0) {
            printf("seek right:%s\r\n", lteRxBuf);
        } else {
            printf("seek error:%s\r\n", lteRxBuf);
            //return -1;//seek��OKһֱ����error
        }

        memset(buf, 0, 128);
        memset(str_len, 0, 5);
        strcpy(buf, "AT+QFREAD=");
        strcat(buf, FileHandle);
        strcat(buf, ",");
        sprintf(str_len,"%d",DOWNLOADLEN);//��ȡ1024����
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
            printf("��ȡ�ļ�error:%s\r\n", lteRxBuf);
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

//�ж��ź�����
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

//�����ļ���
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

















