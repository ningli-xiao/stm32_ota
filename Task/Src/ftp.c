#include "string.h"
#include <stdio.h>
#include "bsp/stmflash/stm_flash.h"
#include "bsp/spi/bsp_spi_flash.h"
#include "bsp/ftp/ftp.h"
#include "bsp/usart/bsp_debug_usart.h"

#define USERNAME "bicycleftp"
#define PASSWORD "34tjYXBw"

#define FTPSERVER "123.7.182.34"
#define SERVERPORT 30021


#define TIME_IN   200
#define TIME_OUT  1000
#define HEADLEN	  96

u32 app_size=0;//bin文件大小
u16 app_version=0;//版本号
u8 updata_flag=0;//升级标志
u8 app_md5[17]={0xe3,0xd8,0xda,0x3d,0xf2,0x27,0x6d,0x63,0x82,0x08,0x46,0x02,0x20,0x5f,0x87,0x8f,0x00};//MD5值
char handle[10]={0};//手柄初始值
u8 Lte_RX_ok=0;
u8 Lte_RX_BUF[LTE_USART_REC_LEN]={0};
u8 error_count=0;
u16 wLteUsartReceiveSize=0;

//外部函数声明
void delay(u32 time);
char* Int2String(int num,char *str)//10进制
{
    int i = 0;//指示填充str
	int j = 0;
    if(num<0)//如果num为负数，将num变正
    {
        num = -num;
        str[i++] = '-';
    }
    //转换
    do
    {
        str[i++] = num%10+48;//取num最低位 字符0~9的ASCII码是48~57；简单来说数字0+48=48，ASCII码对应字符'0'
        num /= 10;//去掉最低位
    }
    while(num); //num不为0继续循环

    str[i] = '\0';

    //确定开始调整的位置
    
    if(str[0]=='-')//如果有负号，负号不用调整
    {
        j = 1;//从第二位开始调整
        ++i;//由于有负号，所以交换的对称轴也要后移1位
    }
    //对称交换
    for(; j<i/2; j++)
    {
        //对称交换两端的值 其实就是省下中间变量交换a+b的值：a=a+b;b=a-b;a=a-b;
        str[j] = str[j] + str[i-1-j];
        str[i-1-j] = str[j] - str[i-1-j];
        str[j] = str[j] - str[i-1-j];
    }

    return str;//返回转换后的值
}

void LTE_GPIO_Init(void)
{
 GPIO_InitTypeDef  GPIO_InitStructure;
 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC, ENABLE);
	
 GPIO_InitStructure.GPIO_Pin = IO_LTE_ACTIVATION_CONTROL_PIN;	    
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
 GPIO_Init(IO_LTE_ACTIVATION_CONTROL_PORT, &GPIO_InitStructure);				
 GPIO_ResetBits(IO_LTE_ACTIVATION_CONTROL_PORT, IO_LTE_ACTIVATION_CONTROL_PIN);

 GPIO_InitStructure.GPIO_Pin = IO_EN_VBAT_Control_PIN;	    
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
 GPIO_Init(IO_EN_VBAT_VCC_Control_PORT, &GPIO_InitStructure);				
 GPIO_ResetBits(IO_EN_VBAT_VCC_Control_PORT, IO_EN_VBAT_Control_PIN);
	
	GPIO_InitStructure.GPIO_Pin = IO_WAKE_Control_PIN;	    
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
 GPIO_Init(IO_WAKE_Control_PORT, &GPIO_InitStructure);				
 GPIO_SetBits(IO_WAKE_Control_PORT, IO_WAKE_Control_PIN);
}

static void Lte_DMA_config(void)
{
   DMA_InitTypeDef    DMA_Initstructure;
   NVIC_InitTypeDef   NVIC_Initstructure;	

	DMA_DeInit(DMA1_Channel3);
	
   RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
   DMA_Initstructure.DMA_PeripheralBaseAddr =  (u32)(&USART3->DR);;
   DMA_Initstructure.DMA_MemoryBaseAddr = (u32)Lte_RX_BUF;
   DMA_Initstructure.DMA_DIR = DMA_DIR_PeripheralSRC;
   DMA_Initstructure.DMA_BufferSize = LTE_USART_REC_LEN;  
   DMA_Initstructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
   DMA_Initstructure.DMA_MemoryInc =DMA_MemoryInc_Enable;
   DMA_Initstructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
   DMA_Initstructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
   DMA_Initstructure.DMA_Mode = DMA_Mode_Normal;
   DMA_Initstructure.DMA_Priority = DMA_Priority_High;
   DMA_Initstructure.DMA_M2M = DMA_M2M_Disable;
	
   DMA_Init(DMA1_Channel3,&DMA_Initstructure); 
   USART_DMACmd(USART3,USART_DMAReq_Rx,ENABLE);//只使用DMA接受功能
   DMA_Cmd(DMA1_Channel3,ENABLE);   
}

static void USART3_NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure; 
	
	NVIC_InitStructure.NVIC_IRQChannel = LTE_USART3_IRQn;	 
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void Lte_USART3_Init(u32 bound)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
  USART3_NVIC_Configuration();
  LTE_USART3_ClockCmd(LTE_USART3_CLK,ENABLE);
  LTE_USART3_GPIO_ClockCmd(LTE_USART3_TX_CLK | LTE_USART3_RX_CLK | RCC_APB2Periph_AFIO,ENABLE);
  GPIO_PinRemapConfig(GPIO_PartialRemap_USART3,ENABLE); 
	USART_DeInit(LTE_USART3);
	GPIO_InitStructure.GPIO_Pin =  LTE_USART3_TX_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LTE_USART3_TX_PORT, &GPIO_InitStructure);  
	
	GPIO_InitStructure.GPIO_Pin = LTE_USART3_RX_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(LTE_USART3_RX_PORT, &GPIO_InitStructure);	

	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		

	USART_Init(LTE_USART3, &USART_InitStructure);
  USART_ITConfig(LTE_USART3, USART_IT_IDLE, ENABLE);	

	USART_Cmd(LTE_USART3, ENABLE);
	Lte_DMA_config();
   
	USART_ClearFlag(LTE_USART3, USART_FLAG_TC|USART_FLAG_TXE|USART_FLAG_RXNE);
}

void Usart3_SendByte(char ch)
{
  USART_SendData(LTE_USART3,ch);
  while (USART_GetFlagStatus(LTE_USART3, USART_FLAG_TXE) == RESET);	
}


void Usart3_SendStr_length(uint8_t *str,uint32_t strlen)
{
  unsigned int k=0;
  do 
  {
    Usart3_SendByte(*(str + k));
    k++;
  } while(k < strlen);
}


void Usart3_SendString(char *str)
{
	unsigned int k=0;
  do 
  {
    Usart3_SendByte(*(str + k));
    k++;
  } while(*(str + k)!='\0');
}

void LTE_UART_SendData(char *pdatabuf)
{
    Usart3_SendString(pdatabuf);
    while(USART_GetFlagStatus(LTE_USART3,USART_FLAG_TC)!=SET);
}

void LTE_UART_SendArrayData(u8 *pdatabuf,u32 nbchar )
{
    Usart3_SendStr_length(pdatabuf,nbchar);
    while(USART_GetFlagStatus(LTE_USART3,USART_FLAG_TC)!=SET);
}
#if 1

void LTE_USART3_IRQHANDLER(void)
{
	u8 uc=0;
	
	if(USART_GetITStatus(LTE_USART3, USART_IT_IDLE) != RESET)  
	{
		USART_ClearITPendingBit(LTE_USART3,USART_IT_IDLE);
		uc = USART3->SR;
        uc = USART3->DR; 
		wLteUsartReceiveSize = LTE_USART_REC_LEN - DMA_GetCurrDataCounter(DMA1_Channel3);		
		Lte_RX_ok=1;
	}
}
#endif

/*******************************************************************************
* Function Name  : LteActivation
* Description    : 移远模组开机
* Input          : None
* Return         : None
*******************************************************************************/
void LteActivation(void)
{
	if(Send_FTPATCommand("ati\r\n","OK",TIME_IN,NULL)==0)//无回复未处于开机状态
	{
		GPIO_SetBits(IO_LTE_ACTIVATION_CONTROL_PORT, IO_LTE_ACTIVATION_CONTROL_PIN);	
		delay(2000);
		GPIO_ResetBits(IO_LTE_ACTIVATION_CONTROL_PORT, IO_LTE_ACTIVATION_CONTROL_PIN);	
	}
}

/************************************************************
** 函数名称:   findstrfromem
** 功能描述: 从内存中查找指定字符串
** 返 回 值:
** 作     者:qifei
** 日     期:2020.8.06
**************************************************************/
char *findstrfromem(char *buf, u16 buflen, char *str)
{
    u8 len = strlen(str);
    u8 ret;
    u32 temp = 0;
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
static void ftp_delay(u32 time)
{
	while(time--)
	{}
}
/************************************************************
** 函数名称:   Send_FTPATCommand
** 功能描述: 发送FTP的at指令
** 返 回 值:
** 作     者:qifei
** 日     期:2020.8.06
**************************************************************/
char* Send_FTPATCommand(char *pCommand, char *pEcho, u32 outTime, u32 *return_len)
{
    u32 relen=0;
    char *pRet=NULL;
	
	DMA_Cmd(DMA1_Channel3,DISABLE);
	DMA1_Channel3->CNDTR=LTE_USART_REC_LEN;       
	DMA_Cmd(DMA1_Channel3,ENABLE); 	
	
   if(NULL!=pCommand)
   {	
		Lte_RX_ok=0;
		memset(Lte_RX_BUF,0,LTE_USART_REC_LEN);
		LTE_UART_SendData((char *)pCommand);
   }
   while(--outTime)//等待指令回复
   {
      if(1==Lte_RX_ok)
      {
			Lte_RX_ok=0;
        	pRet=findstrfromem((char *)Lte_RX_BUF,LTE_USART_REC_LEN,pEcho);
			if(pRet!=NULL)
			{
				return pRet;
			}
      }
	 delay(10);
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
char ftpserver_config()
{
    char *rx_buf = NULL;
    rx_buf = Send_FTPATCommand("ATE0\r\n", "OK", TIME_IN, NULL);
    if (rx_buf == NULL) {
		printf("配置失败0\r\n");
        return -1;
    }

    rx_buf = Send_FTPATCommand("AT+QIDEACT=1\r\n", "OK", TIME_IN, NULL);
    if (rx_buf == NULL) {
			printf("配置失败1\r\n");
    }

    rx_buf = Send_FTPATCommand("AT+QIACT=1\r\n", "OK", TIME_IN, NULL);
    if (rx_buf == NULL) {
			printf("配置失败2\r\n");
        return -1;
    }

    rx_buf = Send_FTPATCommand("AT+QFTPCFG=\"contextid\",1\r\n", "OK", TIME_IN, NULL);
    if (rx_buf == NULL) {
			printf("配置失败3\r\n");
        return -1;
    }

    rx_buf = Send_FTPATCommand("AT+QFTPCFG=\"account\",\"bicycleftp\",\"34tjYXBw\"\r\n", "OK", TIME_IN, NULL);
    if (rx_buf == NULL) {
			printf("配置失败4\r\n");
        return -1;
    }

    rx_buf = Send_FTPATCommand("AT+QFTPCFG=\"filetype\",0\r\n", "OK", TIME_IN, NULL);
    if (rx_buf == NULL) {
			printf("配置失败5\r\n");
        return -1;
		}
    rx_buf = Send_FTPATCommand("AT+QFTPCFG=\"transmode\",1\r\n", "OK", TIME_IN, NULL);
    if (rx_buf == NULL) {
			printf("配置失败6\r\n");
        return -1;
    }

    rx_buf = Send_FTPATCommand("AT+QFTPCFG=\"rsptimeout\",180\r\n", "OK", TIME_IN, NULL);
    if (rx_buf == NULL) {
			printf("配置失败7\r\n");

        return -1;
    }
	return 0;
	printf("配置成功\r\n");
}

/************************************************************
** 函数名称:   ftpserver_login
** 功能描述: 登录
** 返 回 值:
** 作     者:qifei
** 日     期:2020.8.06
**************************************************************/
char ftpserver_login()
{
    char *rx_buf = NULL;
    rx_buf = Send_FTPATCommand("AT+QFTPOPEN=\"123.7.182.34\",30021\r\n", "+QFTPOPEN: 0,", TIME_OUT, NULL);
    if (rx_buf == NULL) {
		printf("注册失败\r\n");
		printf("%s",Lte_RX_BUF);
        return -1;	
    }
	else{
		printf("注册成功\r\n");
		printf("%s",Lte_RX_BUF);
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
void ftpserver_logout()
{
    char *rx_buf = NULL;

    rx_buf = Send_FTPATCommand("AT+QFTPCLOSE\r\n", "+QFTPCLOSE: 0,", TIME_OUT, NULL);
    if (rx_buf == NULL) {
        return ;
    }
}

/************************************************************
** 函数名称:   getfile_list
** 功能描述: 获取文件列表
** 返 回 值:
** 作     者:lixiaoning
** 日     期:2021.6.28
**************************************************************/
u32 getfile_list()
{

}

/************************************************************
** 函数名称:   getfile_size
** 功能描述: 获取FTP服务中指定文件的大小
** 返 回 值:
** 作     者:qifei
** 日     期:2020.8.06
**************************************************************/
u32 getfile_size()
{
	char file_size[100] = "AT+QFTPSIZE=\"G30L_4G_V";
   char *rx_buf = NULL;
    u32 len = 0;
	 u8 str_len[5] = {0};
	 u8 atlen = strlen(file_size);
	
	 memset(file_size + atlen, 0, 20);
    strcat(file_size, Int2String(app_version, str_len));
    strcat(file_size, ".bin\"");
    strcat(file_size, "\r\n");
		
    rx_buf = Send_FTPATCommand("AT+QFTPCWD=\"/G30L\"\r\n", "+QFTPCWD: 0,", TIME_OUT, NULL);
    if (rx_buf == NULL) {
			printf("%s",Lte_RX_BUF);
			printf("文件长度0\r\n");
        return 0;
    }
    rx_buf = Send_FTPATCommand(file_size, "+QFTPSIZE: 0,", TIME_OUT, NULL);
    if (rx_buf == NULL) {
			printf("%s",Lte_RX_BUF);
			printf("文件长度1\r\n");
        return 0;
    }
    len = atoi(rx_buf + 13);
	  printf("文件长度0：%d\r\n",len);
    return len;	
}
/************************************************************
** 函数名称:   getfile_head
** 功能描述: 获取文件头部信息 共96字节
** 返 回 值:
** 作     者:qifei
** 日     期:2020.8.06
**************************************************************/


char getfile_headmd5(u8* md5)
{
    char *rx_buf = NULL;
	char *rx_handle = NULL;
	char str_len[5] = {0};	
	char atcmd[100] = {0};
  u8 atlen = 0;
	u8 temp;
	rx_buf=Send_FTPATCommand("AT+QFOPEN=\"G30L.bin\",0\r\n", "OK", TIME_IN, NULL);
	if(rx_buf!=NULL)
	{
		printf("打开成功:%s\r\n",Lte_RX_BUF);	
		
	}
	else{
		printf("打开失败:%s\r\n",Lte_RX_BUF);
		return -1;
	}
	rx_handle=findstrfromem((char*) &Lte_RX_BUF[0], 200, "+QFOPEN:");
	
	if(rx_handle!=NULL)
	{
		rx_handle+=9;
		memcpy(&handle[0],rx_handle, (u8)(rx_buf-rx_handle-4));//减去两个0D0A
		printf("get handle right:%s\r\n",handle);	
	}
	else{
		printf("get handle error:%s\r\n",Lte_RX_BUF);
		return -1;
	}
	
	memset(atcmd, 0, 20);
	memcpy(atcmd,"AT+QFREAD=",10);
	atlen=strlen(atcmd);
	memset(atcmd + atlen, 0, 20);
	strcat(atcmd, handle);
	strcat(atcmd, ",96\r\n");
	rx_buf=Send_FTPATCommand(atcmd, "OK", TIME_IN, NULL);

	if(rx_buf!=0)
	{
		rx_buf=findstrfromem((char*) &Lte_RX_BUF[0], LTE_USART_REC_LEN, "CONNECT");
		if (rx_buf!=0)
		{
			rx_buf+=9;
			printf("读取MD5 ok:%s\r\n",Lte_RX_BUF);
			memcpy(md5,rx_buf+80,16);
		}
		else{
			printf("MD5 no connect:%s\r\n",Lte_RX_BUF);
			return -1;
		}
		
	}
	else{
		printf("读取MD5 error:%s\r\n",Lte_RX_BUF);
		return -1;
	}
	return 0;
}

/************************************************************
** 函数名称:     deletefile
** 功能描述: 删除UFS里存储的文件
** 返 回 值: 结果
** 作     者:lixiaoning
** 日     期:2021.6.28
**************************************************************/
char deletefile()
{
	if(0!=Send_FTPATCommand("AT+QFDEL=\"*\"\r\n","OK", TIME_OUT, NULL))
	{
		 printf("删除成功%s\r\n",Lte_RX_BUF);
		 return 0;
	}
	return -1;
}

/************************************************************
** 函数名称:   downloadfile
** 功能描述: 从FTP下载文件，每次下载1024字节
** 返 回 值:
** 作     者:qifei
** 日     期:2020.8.05
**************************************************************/
char downloadfile()
{
	char error_count=0;
	char *rx_buf = NULL;
    char str_len[5] = {0};
	char atcmd[100] = "AT+QFTPGET=\"G30L_4G_V";
	char handle[10] = {0};
	u8 temp=0;
    u8 atlen = strlen(atcmd);

    memset(atcmd + atlen, 0, 30);
	strcat(atcmd, Int2String(app_version, str_len));
	strcat(atcmd, ".bin\",");
	strcat(atcmd, "\"UFS:G30L.bin\"\r\n");

    rx_buf = Send_FTPATCommand(atcmd, "OK", TIME_OUT, NULL);
    if (rx_buf != NULL){
            printf("下载成功%s\r\n",Lte_RX_BUF);
			return 0;
    }else{
			printf("下载失败%s\r\n",Lte_RX_BUF);
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
char writefile(u32 len)
{
	static u16 count=0;
	char *rx_buf = NULL;
	char str_len[5] = {0};	
	char atcmd[100] = {0};
	u8 atlen = 0;
	u32 time=0;
	u32 rest_flag=0;
	
	time=len/DOWNLOADLEN;
	rest_flag=len % DOWNLOADLEN;//是否有剩余包
	
	if(count<time){
		memset(atcmd, 0, 20);
		memcpy(atcmd,"AT+QFSEEK=",10);
		atlen=strlen(atcmd);
		memset(atcmd + atlen, 0, 20);
	
		strcat(atcmd, handle);
		strcat(atcmd, ",");
		strcat(atcmd, Int2String(HEADLEN+count * DOWNLOADLEN, str_len));
		strcat(atcmd, "\r\n");
		
		rx_buf=Send_FTPATCommand(atcmd, Int2String(HEADLEN+count * DOWNLOADLEN, str_len), TIME_IN, NULL);
	
		if(rx_buf!=0)
		{
			printf("seek right:%s\r\n",Lte_RX_BUF);
		}
		else{
			printf("seek error:%s\r\n",Lte_RX_BUF);
		}

		memset(atcmd, 0, 20);
		memcpy(atcmd,"AT+QFREAD=",10);
		atlen=strlen(atcmd);
		memset(atcmd + atlen, 0, 20);
		strcat(atcmd, handle);
		
		strcat(atcmd, ",");
		strcat(atcmd, Int2String(DOWNLOADLEN, str_len));
		strcat(atcmd, "\r\n");

		rx_buf=Send_FTPATCommand(atcmd, "OK", TIME_OUT, NULL);
		if(rx_buf!=0)
		{
			rx_buf=findstrfromem((char *)Lte_RX_BUF,LTE_USART_REC_LEN,"CONNECT");
			if(rx_buf!=0){
				rx_buf+=9;
				SPI_FLASH_BufferWrite(rx_buf,SPI_FLASH_WriteAddress+count *DOWNLOADLEN, DOWNLOADLEN);
			}
			else{
				printf("find not connect%s\r\n");
				return -1;
			}
		}
		else{
			printf("读取文件error:%s\r\n",Lte_RX_BUF);
			return -1;
		}
	}
	
	if(count==time&&rest_flag!=0)//有剩余包且到达指定位置
	{
		rest_flag=0;
		memset(atcmd, 0, 20);
		memcpy(atcmd,"AT+QFSEEK=",10);
		atlen=strlen(atcmd);
		memset(atcmd + atlen, 0, 20);
	
		strcat(atcmd, handle);
		strcat(atcmd, ",");
		strcat(atcmd, Int2String(HEADLEN+count * DOWNLOADLEN, str_len));
		strcat(atcmd, "\r\n");
		
		rx_buf=Send_FTPATCommand(atcmd, Int2String(HEADLEN+count * DOWNLOADLEN, str_len), TIME_IN, NULL);
	
		if(rx_buf!=0)
		{
			printf("seek right:%s\r\n",Lte_RX_BUF);
		}
		else{
			printf("seek error:%s\r\n",Lte_RX_BUF);
			//return -1;//seek的OK一直都是error
		}
		
		memset(atcmd, 0, 20);
		memcpy(atcmd,"AT+QFREAD=",10);
		atlen=strlen(atcmd);
		memset(atcmd + atlen, 0, 20);
		strcat(atcmd, handle);
		
		strcat(atcmd, ",");
		strcat(atcmd, Int2String(len % DOWNLOADLEN, str_len));
		strcat(atcmd, "\r\n");

		rx_buf=Send_FTPATCommand(atcmd, "OK", TIME_OUT, NULL);

		if(rx_buf!=0)
		{
			rx_buf=findstrfromem((char *)Lte_RX_BUF,LTE_USART_REC_LEN,"CONNECT");
			if(rx_buf!=0){
				rx_buf+=9;
				SPI_FLASH_BufferWrite(rx_buf,SPI_FLASH_WriteAddress+count *DOWNLOADLEN, DOWNLOADLEN);
			}
			else{
				printf("find not connect%s\r\n",Lte_RX_BUF);
				return -1;
			}
		}
		else{
			printf("读取文件error:%s\r\n",Lte_RX_BUF);
			return -1;
		}	
		
		memset(atcmd, 0, 20);
		memcpy(atcmd,"AT+QFCLOSE=",11);
		atlen=strlen(atcmd);
		memset(atcmd + atlen, 0, 20);
		strcat(atcmd, handle);
		strcat(atcmd, "\r\n");	
		if(0!=Send_FTPATCommand(atcmd, "OK", TIME_IN, NULL))
		{
			printf("关闭结果:%s\r\n",Lte_RX_BUF);	
		}
		else{
			printf("关闭error\r\n");
			return -1;
		}
	}							
	count++;
	return count;
}

int Wait_LTE_RDY(u8 time)
{
    while(--time)
    {
        if(Send_FTPATCommand(NULL, "RDY",TIME_IN,NULL) == 0)
        {
            printf(" No  RDY \r\n");
        }
        else
        {
            printf(" RDY Come\r\n");
            return 0;
        }
		delay(1000);
    }
    printf(" Wait_LTE_RDY Timeout\r\n");
    return -1;
}
//判断信号质量
int LTE_Signal_Quality(void)
{
    char *pRet=0;
    pRet = Send_FTPATCommand("AT+CSQ\r\n","OK",TIME_IN,NULL);
    if(pRet == 0)
    {
        return -1;
    }
    pRet =(char *)&Lte_RX_BUF[2];
    printf(" LTE_Signal_Quality pRet=%s\r\n",pRet);
    if(findstrfromem((char *)pRet, LTE_USART_REC_LEN,"99,99") != 0)
    {
        return -1;
    }
    return 0;
}

int Wait_Signal_RDY(u8 time)
{
    while(--time)
    {
        if(LTE_Signal_Quality() != 0)
        {
            printf(" No  Signal \r\n");
        }
        else
        {
            printf(" Signal OK\r\n");
            return 0;
        }
        delay(1000);
    }
    printf(" Wait_Signal_RDY Timeout\r\n");
    return -1;
}

















