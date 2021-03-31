#include "stm32f0xx.h"
#include "stdio.h"
#include "stdarg.h"
#include "string.h"

#ifndef BIT
#define BIT(x)	(1 << (x))
#endif
#define RESET_IO(x)     GPIO_WriteBit(GPIOA, GPIO_Pin_0, (BitAction)x)
//上一次时间戳
char sw_ts[32] = {0};
#define RESET_BUTTON(x)     GPIO_WriteBit(GPIOA, GPIO_Pin_1, (BitAction)x);
#define PRESS_BUTTON()   		RESET_BUTTON(1);
#define RELEASE_BUTTON()		RESET_BUTTON(0);

//复位 0:不需要 1:需要复位
char Reset_Status = 1;

//轮询 0:时间未到 1:开始轮询 2:等待回应
char Poll_Status = 0;

//连接 0:未连接 1:已连接
char Conn_Status = 0;

#define USART1_TXBUFF_SIZE   512
#define USART1_RXBUFF_SIZE   512
unsigned int USART1_RxCounter;
char USART1_RxCompleted;
char USART1_TxBuff[USART1_TXBUFF_SIZE];
char USART1_RxBuff[USART1_RXBUFF_SIZE];

#define WiFi_RxCounter    USART1_RxCounter
#define WiFi_RxBuff       USART1_RxBuff
#define WiFi_RxBuff_Size  USART1_RXBUFF_SIZE

void TIM1_Init(unsigned int ms)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	TIM_TimeBaseInitStruct.TIM_Period = (4800-1)*ms/1000;
	TIM_TimeBaseInitStruct.TIM_Prescaler=10000-1;
	TIM_TimeBaseInitStruct.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM1,&TIM_TimeBaseInitStruct);

	TIM_ClearFlag(TIM1, TIM_FLAG_Update);
	TIM_ARRPreloadConfig(TIM1, ENABLE);
	TIM_ClearITPendingBit(TIM1,TIM_IT_Update);
	TIM_ITConfig(TIM1,TIM_IT_Update,ENABLE);
	TIM_Cmd(TIM1,DISABLE);

	NVIC_InitStruct.NVIC_IRQChannel=TIM1_BRK_UP_TRG_COM_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority=0;
	NVIC_InitStruct.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStruct);
}

void TIM3_Init(unsigned int ms)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_TimeBaseInitStruct.TIM_Period = (4800-1)*ms/1000;
	TIM_TimeBaseInitStruct.TIM_Prescaler=10000-1;
	TIM_TimeBaseInitStruct.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStruct);

	TIM_ClearFlag(TIM3, TIM_FLAG_Update);
	TIM_ARRPreloadConfig(TIM3, ENABLE);
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);
	TIM_Cmd(TIM3,DISABLE);

	NVIC_InitStruct.NVIC_IRQChannel=TIM3_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority=0;
	NVIC_InitStruct.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStruct);
}

void TIM14_Init(unsigned int ms)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14, ENABLE);
	TIM_TimeBaseInitStruct.TIM_Period = (4800-1)*ms/1000;
	TIM_TimeBaseInitStruct.TIM_Prescaler=10000-1;
	TIM_TimeBaseInitStruct.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_ClockDivision=TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM14,&TIM_TimeBaseInitStruct);

	TIM_ClearFlag(TIM14, TIM_FLAG_Update);
	TIM_ARRPreloadConfig(TIM14, ENABLE);
	TIM_ClearITPendingBit(TIM14,TIM_IT_Update);
	TIM_ITConfig(TIM14,TIM_IT_Update,ENABLE);
	TIM_Cmd(TIM14,DISABLE);

	NVIC_InitStruct.NVIC_IRQChannel=TIM14_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority=0;
	NVIC_InitStruct.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStruct);
}

void USART1_IRQHandler(void)   
{
	if (USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET) {
		USART_ClearFlag(USART1, USART_FLAG_ORE);
		USART_ReceiveData(USART1); 
	}
	if (USART_GetITStatus(USART1,USART_IT_RXNE) != RESET) {
		if (Conn_Status == 0) {
			if(USART1->RDR){
				USART1_RxBuff[USART1_RxCounter++] = USART_ReceiveData(USART1);
			}
		} else {
			USART1_RxBuff[USART1_RxCounter] = USART_ReceiveData(USART1);
			if (USART1_RxCounter == 0) TIM_Cmd(TIM1, ENABLE);
			else TIM_SetCounter(TIM1, 0);
			USART1_RxCounter++;
		}
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);
	}
}


uint16_t fac_ms;
uint8_t fac_us;
void Delay_Ms(uint16_t nms)
{
	int32_t temp;		   
	SysTick->LOAD=(int32_t)nms*fac_ms;
	SysTick->VAL =0x00;
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;
	do {
		temp=SysTick->CTRL;
	}
	while(temp&0x01&&!(temp&(1<<16)));
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;
	SysTick->VAL =0X00;
}
void Delay_Init()
{
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
	fac_us=SystemCoreClock/8000000;
	fac_ms=(int16_t)fac_us*1000; 
}

void USART1_Init(unsigned int baud)
{  	 	
	GPIO_InitTypeDef GPIO_InitStruct;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStruct;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	//设置PA9/PA10作为TX/RX端口
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_1);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART1, &USART_InitStructure);		
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
	USART_Cmd(USART1, ENABLE);
	
	NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPriority = 0x02;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
}

void RelayIO_Init()
{
	GPIO_InitTypeDef GPIO_InitStruct;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;	
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	//默认低电平，高电平闭合
	RESET_BUTTON(0);
}
void WiFi_Init()
{
	GPIO_InitTypeDef GPIO_InitStruct;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;	
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	RESET_IO(1);
}

void WiFi_Input(char* fmt,...)
{  
	unsigned int i,length;
	va_list ap;
	va_start(ap,fmt);
	vsprintf(USART1_TxBuff,fmt,ap);
	va_end(ap);	
	
	length=strlen((const char*)USART1_TxBuff);		
	while((USART1->ISR&0x80)==0);
	for(i = 0; i < length; i++) {			
		USART1->TDR = USART1_TxBuff[i];
		while((USART1->ISR&0x80)==0);	
	}	
}
char WiFi_SendCmd(char *cmd, int timeout)
{
	WiFi_RxCounter=0;
	memset(WiFi_RxBuff,0,WiFi_RxBuff_Size);
	WiFi_Input("%s\r\n",cmd);
	while(timeout--){
		Delay_Ms(100);
		if(strstr(WiFi_RxBuff,"OK"))
			break;
	}
	if(timeout<=0)return 1;
	else return 0;
}

char WiFi_Reset(int timeout)
{
	int i;
	WiFi_RxCounter=0;
	RESET_IO(0);
	Delay_Ms(500);
	RESET_IO(1);
	while(timeout--){
		Delay_Ms(100);
		for (i = 0; i < USART1_RxCounter; i++)
			if (WiFi_RxBuff[i] == '\0')
				WiFi_RxBuff[i] = 0xff;
		if(strstr(WiFi_RxBuff,"ready"))
			break;
	}
	if(timeout<=0)return 1;
	else return 0;
}


char RXbuff[512];
char TXbuff[512];
//根据实际情况配置
#define ACCESS_KEY "Basic aW90MTpwYXNzd2Qx" //base64(iot1:passwd1)
#define SSID   	   "****"
#define PASSWORD   "****"
#define SERVER_IP  ""
#define SERVER_PORT 0

char WiFi_Connect(int timeout)
{		
	WiFi_RxCounter=0;
	memset(WiFi_RxBuff,0,WiFi_RxBuff_Size);
	WiFi_Input("AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,PASSWORD);
	while(timeout--){
		Delay_Ms(1000);
		if(strstr(WiFi_RxBuff,"WIFI GOT IP\r\n\r\nOK"))
			break;
	}
	if(timeout<=0) return 1;
	return 0;
}

char WiFi_Connect_Server(int timeout)
{	
	WiFi_RxCounter=0;
	memset(WiFi_RxBuff,0,WiFi_RxBuff_Size);
	WiFi_Input("AT+CIPSTART=\"TCP\",\"%s\",%d\r\n",SERVER_IP,SERVER_PORT);
	while(timeout--){
		Delay_Ms(100);
		if(strstr(WiFi_RxBuff ,"CONNECT"))
			break;
		if(strstr(WiFi_RxBuff ,"CLOSED"))
			return 1;
		if(strstr(WiFi_RxBuff ,"ALREADY CONNECTED"))
			return 1;
	}
	if(timeout<=0)
		return 1;
	WiFi_RxCounter=0;
	memset(WiFi_RxBuff,0,WiFi_RxBuff_Size);
	WiFi_Input("AT+CIPSEND\r\n");
	while(timeout--){
		Delay_Ms(100);
		if(strstr(WiFi_RxBuff,"\r\nOK\r\n\r\n>"))
			break;
	}
	if(timeout<=0) return 1;
	return 0;
}


char WiFi_Close(int timeout)
{		
  Delay_Ms(200);
	WiFi_Input("+++");
	Delay_Ms(1500);
	Conn_Status = 0;
	WiFi_RxCounter=0;                  
	memset(WiFi_RxBuff,0,WiFi_RxBuff_Size);
	WiFi_Input("AT+CIPCLOSE\r\n");
	while(timeout--){
		Delay_Ms(100);
		if(strstr(WiFi_RxBuff,"OK"))
			break;
	}
	if(timeout<=0) return 1;
	return 0;
}

//构造HTTP请求开关数据
void WiFi_MiniSW_Status()
{
	char temp[128];
	memset(TXbuff,0,sizeof(TXbuff));
  memset(temp,0,sizeof(temp));                                         
	sprintf(TXbuff,"GET /minisw/ HTTP/1.1\r\n");
	sprintf(temp,"Authorization:%s\r\n",ACCESS_KEY);
	strcat(TXbuff,temp);
	strcat(TXbuff,"Host:www.aaabbbccc.com:30000\r\n\r\n");
}

char WiFi_Start()
{
	//复位
	if (WiFi_Reset(50)) {
		return 1;
	}
	//设置STA模式
	if (WiFi_SendCmd("AT+CWMODE=1",50)) {
		return 1;
	}
	//取消自动连接
	if (WiFi_SendCmd("AT+CWAUTOCONN=0",50)) {
		return 1;
	}
	//开始连接
	if (WiFi_Connect(30)){
		return 1;
	}
	//开启透传
	if(WiFi_SendCmd("AT+CIPMODE=1",50)){
		return 1;
	}
	//关闭多路连接
	if(WiFi_SendCmd("AT+CIPMUX=0",50)){
		return 1;
	}
	return 0;
}

void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET){
		USART1_RxCompleted = 1;
		memcpy(&RXbuff[2],USART1_RxBuff,USART1_RxCounter);
		RXbuff[0] = WiFi_RxCounter/256;
		RXbuff[1] = WiFi_RxCounter%256;
		RXbuff[WiFi_RxCounter+2] = '\0';
		WiFi_RxCounter=0;
		TIM_Cmd(TIM1, DISABLE);
		TIM_SetCounter(TIM1, 0);
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	}
}

void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		Reset_Status = 1;
		Poll_Status = 0;
		TIM_Cmd(TIM3, DISABLE);
		TIM_Cmd(TIM14, DISABLE);
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	}
}

void TIM14_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM14, TIM_IT_Update) != RESET) {
		Poll_Status = 1;
		TIM_Cmd(TIM14, DISABLE);
		TIM_ClearITPendingBit(TIM14, TIM_IT_Update);
	}
}


int main(void)
{
	//延时初始化
	Delay_Init();
	
	//WiFi初始化
	WiFi_Init();
	
	//继电器IO口初始化
	RelayIO_Init();
	
	//串口初始化
	USART1_Init(115200);
	
	//LED初始化
	//LED_Init();
	//LED_OFF();
		
	//接收数据定时器
	TIM1_Init(50);
	//接收超时定时器
	TIM3_Init(5000);
	//轮询定时器
	TIM14_Init(3000);

	while(1) {
		
		if (Reset_Status == 1) {
			//重置WiFi连接
			if (!WiFi_Start()) {
				//WiFi启动成功，开始轮询
				TIM_Cmd(TIM14, ENABLE);
				Reset_Status = 0;
			}
		}
		
		if (USART1_RxCompleted == 1) {
			char *json = &RXbuff[2];
			char *presult;
			USART1_RxCompleted = 0;
			TIM_Cmd(TIM3, DISABLE);
			if (strstr(json,"200 OK")) {
				//请求数据成功，检查返回数据
				if(strstr(&RXbuff[2],"\"act\"")){
					  char *p1 = NULL;
					  char *p2 = NULL;
						char *p3 = NULL;
					  char *s2 = "\"ts\": \"";
						p1 = strstr(json,s2);
						if (p1) {
							p2 = p1 + strlen(s2);
							p3 = strstr(p2,"\"");
							if (p3) {
								char ts[32] = {0};
								memset(ts, 0, sizeof(sw_ts));
								memcpy(ts, p2, p3-p2);
								//时间戳改变，说明控制命令有更新
								if (memcmp(ts, sw_ts, sizeof(ts))) {
									if (sw_ts[0] != '\0') {
										presult = strstr(json,"\"act\": "); 
										if(presult!=NULL){
											if(*(presult+7) == '1') {
												//闭合1s，实现关机或开机
												PRESS_BUTTON();
												Delay_Ms(1000);
												RELEASE_BUTTON();
											} else {
												//闭合5s，实现强制关机(重复5次，这样写是防止用5000导致溢出结果不准确)
												PRESS_BUTTON();
												Delay_Ms(1000);
												Delay_Ms(1000);
												Delay_Ms(1000);
												Delay_Ms(1000);
												Delay_Ms(1000);
												RELEASE_BUTTON();
											}
										}									
									}
									memcpy(sw_ts, ts, sizeof(ts));									
								}
							}
						}
				}				
			}
			if(!WiFi_Close(50)) {
				if (Poll_Status == 2) {
					Poll_Status = 0;
					TIM_Cmd(TIM14, ENABLE);
				}
			} else {
				//重置WiFi
				TIM_Cmd(TIM3, DISABLE);
				TIM_Cmd(TIM14, DISABLE);
				Reset_Status = 1;
				Poll_Status = 0;
			}
		}
		
		if (Poll_Status == 1) {
			Poll_Status = 2;
			//连接服务器
			if (!WiFi_Connect_Server(50)) {
				//连接成功，HTTP请求数据
				WiFi_RxCounter = 0;
				memset(WiFi_RxBuff,0,WiFi_RxBuff_Size);
				Conn_Status = 1;
				WiFi_MiniSW_Status();
				WiFi_Input(TXbuff);
				TIM_Cmd(TIM3, ENABLE);
			} else {
				//失败重置
				TIM_Cmd(TIM3, DISABLE);
				TIM_Cmd(TIM14, DISABLE);
				Conn_Status = 0;
				Reset_Status = 1;
				Poll_Status = 0;
			}
		}
	}
}



